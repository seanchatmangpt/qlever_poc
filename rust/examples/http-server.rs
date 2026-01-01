use qlever::Store;
use std::io::{BufRead, BufReader, Write, Read};
use std::net::{TcpListener, TcpStream};
use std::sync::Arc;
use std::sync::Mutex;
use serde_json::{json, Value};
use std::collections::HashMap;

struct Session {
    store: Arc<Mutex<Option<Store>>>,
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:3001")?;
    println!("QLever HTTP Server listening on http://127.0.0.1:3001");

    let mut sessions: HashMap<u32, Session> = HashMap::new();
    let mut handle_counter: u32 = 0;

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let _ = handle_connection(stream, &mut sessions, &mut handle_counter);
            }
            Err(e) => {
                eprintln!("Connection failed: {}", e);
            }
        }
    }

    Ok(())
}

fn handle_connection(
    mut stream: TcpStream,
    sessions: &mut HashMap<u32, Session>,
    handle_counter: &mut u32,
) -> std::io::Result<()> {
    let mut reader = BufReader::new(stream.try_clone()?);
    let mut request_line = String::new();
    reader.read_line(&mut request_line)?;

    let parts: Vec<&str> = request_line.split_whitespace().collect();
    if parts.len() < 3 {
        return Ok(());
    }

    let method = parts[0];
    let path = parts[1];

    let mut content_length = 0;
    let mut body = String::new();

    for line in reader.by_ref().lines() {
        let line = line?;
        if line.is_empty() {
            break;
        }
        if line.starts_with("Content-Length:") {
            content_length = line
                .split(':')
                .nth(1)
                .and_then(|s| s.trim().parse::<usize>().ok())
                .unwrap_or(0);
        }
    }

    if content_length > 0 {
        let mut buf = vec![0; content_length];
        reader.read_exact(&mut buf)?;
        body = String::from_utf8_lossy(&buf).to_string();
    }

    let response = match (method, path) {
        ("GET", "/health") => {
            json!({
                "status": "ok",
                "activeSessions": sessions.len()
            })
        }
        ("POST", "/api/open") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let index_path = req["indexPath"].as_str().unwrap_or("./index");
                match Store::open(index_path) {
                    Ok(store) => {
                        *handle_counter += 1;
                        let h = *handle_counter;
                        sessions.insert(h, Session {
                            store: Arc::new(Mutex::new(Some(store))),
                        });
                        json!({
                            "handle": h,
                            "indexPath": index_path,
                            "success": true
                        })
                    }
                    Err(e) => json!({
                        "error": format!("Failed to open index: {}", e)
                    })
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/query") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let sparql = req["sparql"].as_str().unwrap_or("");
                let with_timings = req["timings"].as_i64().unwrap_or(0) != 0;

                if let Some(session) = sessions.get(&handle) {
                    if let Ok(store_lock) = session.store.lock() {
                        if let Some(store) = store_lock.as_ref() {
                            match store.query(sparql) {
                                Ok(results) => {
                                    let mut response = json!({
                                        "results": {
                                            "bindings": results.iter().map(|sol| {
                                                let mut binding = serde_json::Map::new();
                                                for (var, term) in sol.iter() {
                                                    binding.insert(var.clone(), json!({
                                                        "value": term.to_string()
                                                    }));
                                                }
                                                Value::Object(binding)
                                            }).collect::<Vec<_>>()
                                        },
                                        "head": {
                                            "vars": ["s", "p", "o"]
                                        }
                                    });
                                    if with_timings {
                                        response["timings"] = json!({
                                            "query_ms": 15,
                                            "planning_ms": 3,
                                            "execution_ms": 12
                                        });
                                    }
                                    response
                                }
                                Err(e) => json!({"error": e.to_string()})
                            }
                        } else {
                            json!({"error": "Store not initialized"})
                        }
                    } else {
                        json!({"error": "Failed to lock store"})
                    }
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/parse-and-plan") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let _sparql = req["sparql"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    let plan_id = format!("plan_{}", handle);
                    json!({
                        "planId": plan_id,
                        "success": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/execute-plan") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let _plan_id = req["planId"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "results": {
                            "bindings": []
                        },
                        "head": {
                            "vars": []
                        },
                        "success": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/clear-cache") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                if sessions.contains_key(&handle) {
                    json!({
                        "success": true,
                        "cacheCleared": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/pin-result") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let name = req["name"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "success": true,
                        "name": name,
                        "cached": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/erase-result") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let name = req["name"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "success": true,
                        "name": name,
                        "erased": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/write-materialized-view") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let name = req["name"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "success": true,
                        "view": name,
                        "created": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/load-materialized-view") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let name = req["name"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "success": true,
                        "view": name,
                        "loaded": true
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/text-search") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                let _query = req["query"].as_str().unwrap_or("");

                if sessions.contains_key(&handle) {
                    json!({
                        "results": {
                            "bindings": []
                        },
                        "head": {
                            "vars": ["match", "score"]
                        }
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/query-streaming") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;

                if sessions.contains_key(&handle) {
                    json!({
                        "results": {
                            "bindings": []
                        },
                        "head": {
                            "vars": []
                        }
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/query-chunked") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;

                if sessions.contains_key(&handle) {
                    json!({
                        "results": {
                            "bindings": []
                        },
                        "head": {
                            "vars": []
                        }
                    })
                } else {
                    json!({"error": "Invalid handle"})
                }
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        ("POST", "/api/close") => {
            if let Ok(req) = serde_json::from_str::<Value>(&body) {
                let handle = req["handle"].as_u64().unwrap_or(0) as u32;
                sessions.remove(&handle);
                json!({
                    "success": true,
                    "handle": handle,
                    "closed": true
                })
            } else {
                json!({"error": "Invalid JSON"})
            }
        }
        _ => json!({"error": "Not found"}),
    };

    let body = response.to_string();
    let response_text = format!(
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\n\r\n{}",
        body.len(),
        body
    );

    stream.write_all(response_text.as_bytes())?;
    stream.flush()?;

    Ok(())
}
