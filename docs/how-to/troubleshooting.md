# Troubleshooting Guide

Quick solutions for common QLever problems.

## Installation & Setup

### "qlever: command not found"

**Problem:** The `qlever` CLI tool is not installed.

**Solution:** Install the Python package:
```bash
pip install --upgrade qlever
qlever --version  # Verify
```

**Still not working?**
- Check Python version: `python --version` (need 3.8+)
- Check pip location: `which pip`
- Try: `python3 -m pip install qlever`

---

### "No such file or directory: Qleverfile"

**Problem:** You ran `qlever index` without a Qleverfile.

**Solution:** Create one:
```bash
qlever setup-config wikidata-small
# or
qlever setup-config dblp
```

Then try again:
```bash
qlever index
```

---

## Indexing Issues

### "Out of memory" during `qlever index`

**Problem:** Indexing ran out of RAM.

**Solutions (in order):**

1. **Increase system memory** - Best option
   ```bash
   # Check available RAM
   free -h  # Linux
   vm_stat  # macOS
   ```

2. **Reduce batch size** - Edit Qleverfile:
   ```yaml
   index:
     memory_limit: 4GB
   ```

3. **Use a smaller dataset** - Try sample data first:
   ```bash
   qlever setup-config wikidata-small
   qlever index
   ```

4. **Check for other memory-hungry processes**:
   ```bash
   top  # Linux/macOS
   # Kill unnecessary processes
   ```

---

### "Invalid RDF format" error

**Problem:** Input file has syntax errors.

**Solution:** Validate your RDF:
```bash
# Check file format
file data.ttl

# Try converting format
riot --output=TURTLE data.nt > data.ttl

# Or validate with Python
python -c "
from rdflib import Graph
g = Graph()
g.parse('data.ttl', format='turtle')
print(f'Parsed {len(g)} triples')
"
```

---

### Indexing is very slow

**Problem:** Building the index takes hours.

**Symptoms:**
- Using single CPU core
- Disk activity but low memory usage
- No progress updates

**Solutions:**

1. **Enable parallel parsing** (if using C++ binary):
   ```bash
   IndexBuilderMain -F ttl -f data.ttl -i my-index -p
   ```

2. **Check CPU usage**:
   ```bash
   top -p $(pgrep IndexBuilderMain)
   # If <50% CPU, bottleneck is I/O
   ```

3. **Use faster disk**:
   - Move data to SSD if on HDD
   - Input data location matters more than output

4. **Verify data is actually being parsed**:
   ```bash
   tail -f /tmp/qlever-index.log  # Check logs
   ```

---

## Server & Query Issues

### "Connection refused" when querying

**Problem:** Can't connect to server.

**Check if server is running:**
```bash
# Check if process exists
pgrep ServerMain    # or qlever-server

# Check if port is listening
netstat -tulpn | grep 7023
# or
lsof -i :7023
```

**Solution:** Start the server:
```bash
qlever start
# or
ServerMain -i my-index -p 7023 &
```

---

### "Port already in use"

**Problem:** Can't start server (port in use).

**Find what's using the port:**
```bash
lsof -i :7023
# or
netstat -tulpn | grep 7023
```

**Solutions:**

1. **Use a different port:**
   ```bash
   qlever start --port 8000
   ```

2. **Kill the existing process** (if you own it):
   ```bash
   pkill -f "ServerMain.*7023"
   ```

3. **Wait for it to close:**
   ```bash
   # Port may still be in TIME_WAIT state
   sleep 30
   qlever start
   ```

---

### Query times out

**Problem:** Long-running queries fail.

**Solutions (in order):**

1. **Add more FILTER conditions** - Reduce data early:
   ```sparql
   # Slow: Processes all triples
   SELECT ?x WHERE { ?x a wd:Q5 . ?x wdt:P27 wd:Q30 . }

   # Fast: Filter to US residents directly
   SELECT ?x WHERE {
     ?x wdt:P27 wd:Q30 .
     ?x a wd:Q5 .
   }
   ```

2. **Add LIMIT** - Don't return everything:
   ```sparql
   SELECT ?x WHERE { ... } LIMIT 1000
   ```

3. **Check for cartesian products** - Unconnected patterns:
   ```sparql
   # BAD: ?x and ?y are never linked
   SELECT ?x ?y WHERE {
     ?x a wd:Q5 .
     ?y a wd:Q571 .
   }

   # GOOD: Connected via property
   SELECT ?x ?y WHERE {
     ?x a wd:Q5 .
     ?x wdt:P50 ?y .
     ?y a wd:Q571 .
   }
   ```

4. **Increase timeout** (if using API):
   ```bash
   curl --max-time 600 -Gs http://localhost:7023 \
     --data-urlencode "query=..."
   ```

5. **Increase server memory**:
   ```bash
   qlever stop
   qlever start --memory 32GB
   ```

---

### Query returns wrong results

**Problem:** Results are incorrect or incomplete.

**Check your SPARQL syntax:**
```sparql
# Wrong: Missing period at end
SELECT ?x WHERE { ?x a wd:Q5 }  # ← No period!

# Correct:
SELECT ?x WHERE { ?x a wd:Q5 . }  # ← Period at end
```

**Check your patterns:**
```sparql
# Wrong: Assumes property exists
SELECT ?name WHERE {
  ?person ex:name ?name .
  ?person ex:age ?age .  # What if age doesn't exist?
}

# Correct: Make properties optional
SELECT ?name ?age WHERE {
  ?person ex:name ?name .
  OPTIONAL { ?person ex:age ?age . }
}
```

**Check language filters:**
```sparql
# Missing language filter - gets everything
SELECT ?name WHERE {
  ?person rdfs:label ?name .
}

# Better: Specify language
SELECT ?name WHERE {
  ?person rdfs:label ?name .
  FILTER(LANG(?name) = "en")
}
```

---

### Server crashes

**Problem:** ServerMain crashes or exits unexpectedly.

**Check the error:**
```bash
# If running in background
qlever logs --follow

# If running in foreground, see the output directly
```

**Common causes:**

1. **Out of memory**
   ```bash
   qlever stop
   qlever start --memory 32GB  # Increase memory
   ```

2. **Corrupted index**
   ```bash
   rm my-index.*  # Delete index files
   qlever index   # Rebuild
   ```

3. **Invalid index path**
   ```bash
   ServerMain -i /path/to/my-index -p 7023
   # ^ Must be absolute path or correct relative path
   ```

---

## Docker Issues

### "Cannot connect to Docker daemon"

**Problem:** Docker is not running or not installed.

**Solution:**
```bash
# Start Docker
sudo systemctl start docker  # Linux
open -a Docker               # macOS
# or start via Docker Desktop GUI

# Check Docker is running
docker ps
```

---

### Docker container uses too much memory

**Problem:** Container is killed due to memory pressure.

**Solution:** Limit memory in docker run:
```bash
docker run -m 16GB -p 7023:7001 \
  -e INDEX_PREFIX=wikidata \
  adfreiburg/qlever:latest
```

---

### "Cannot connect to localhost:7023" (in Docker)

**Problem:** Can't reach Docker container.

**Check if container is running:**
```bash
docker ps | grep qlever
```

**Check container logs:**
```bash
docker logs <container-id>
```

**Solution:** Make sure port is published:
```bash
docker run -p 7023:7001 adfreiburg/qlever:latest
#        ^              ^
#        host port      container port
```

---

## Performance Issues

### Queries are slow (>1 second)

**Problem:** Even simple queries take too long.

**Diagnose:**
```bash
# Measure query time
qlever query --show-timing "SELECT ?x WHERE { ?x a ?type } LIMIT 100"

# Check server memory usage
qlever status
```

**Solutions:**

1. **Add FILTER conditions** - Process less data
2. **Increase server memory** - Allow caching
3. **Check index quality** - Rebuild if corrupted
4. **Use different permutations** - May help specific queries

---

### Index files are huge (>100GB)

**Problem:** Index takes up too much disk space.

**Solutions:**

1. **Enable compression** (when rebuilding):
   ```bash
   # In Qleverfile (Python qlever)
   index:
     use_compression: true

   # Then rebuild
   qlever index --force
   ```

2. **Remove unneeded data**:
   - Filter input files before indexing
   - Remove language variants you don't need

3. **Use external prefixes** (C++ binary):
   ```json
   {
     "prefixes-external": [
       "<http://www.wikidata.org/entity/statement/>"
     ]
   }
   ```

---

## I/O & Disk Issues

### "Disk full" error

**Check disk usage:**
```bash
df -h
du -sh my-index*  # Index size
du -sh data.ttl   # Input size
```

**Solution:** Free up space:
```bash
# Move index to larger disk
mv my-index.* /larger/disk/

# Clean old indexes
rm old-index.*
```

---

### "Permission denied" when writing index

**Problem:** No write permission in directory.

**Solution:**
```bash
# Check permissions
ls -la my-index*
chmod 755 .  # Make directory writable
```

---

## Still Stuck?

1. **Check logs**:
   ```bash
   qlever logs --follow
   ```

2. **Try with verbose output**:
   ```bash
   qlever index --show  # See the actual command
   ```

3. **Search for similar issues**:
   - GitHub Issues: https://github.com/ad-freiburg/qlever/issues
   - QLever Wiki: https://github.com/ad-freiburg/qlever/wiki

4. **Open an issue**:
   - Include error message (full output)
   - Include your Qleverfile or command
   - Describe what you're trying to do

---

## Quick Reference Table

| Problem | First Try | If That Fails |
|---------|-----------|---------------|
| Out of memory | Reduce batch size | Increase RAM |
| Port in use | Use different port | Kill existing process |
| Query timeout | Add FILTER/LIMIT | Increase memory |
| Slow queries | Add more FILTERs | Rebuild index |
| Slow indexing | Enable parallel mode | Use faster disk |
| Connection refused | Start server | Check firewall |
| Wrong results | Check SPARQL syntax | Check patterns |

---

**Need more help?** See [Documentation Index](./INDEX.md)
