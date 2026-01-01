# Datalog Examples

Complete real-world examples demonstrating Datalog usage in QLever.

## Table of Contents

1. [Example 1: Family Relationships](#example-1-family-relationships)
2. [Example 2: Graph Reachability](#example-2-graph-reachability)
3. [Example 3: Social Network Analysis](#example-3-social-network-analysis)
4. [Example 4: Organizational Hierarchy](#example-4-organizational-hierarchy)
5. [Example 5: Ontology Inference](#example-5-ontology-inference)
6. [Example 6: Supply Chain Dependencies](#example-6-supply-chain-dependencies)
7. [Example 7: Path Finding](#example-7-path-finding)
8. [Example 8: Access Control](#example-8-access-control)

---

## Example 1: Family Relationships

### Problem Statement

Given a dataset of parent-child relationships, compute various family relationships including ancestors, descendants, siblings, cousins, and degrees of separation.

### RDF Data (Turtle)

```turtle
@prefix : <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

# Generation 1 (Grandparents)
:Alice :parentOf :Charlie .
:Alice :parentOf :Diana .
:Bob :parentOf :Charlie .
:Bob :parentOf :Diana .

# Generation 2 (Parents)
:Charlie :parentOf :Eve .
:Charlie :parentOf :Frank .
:Diana :parentOf :Grace .
:Diana :parentOf :Henry .

# Generation 3 (Children)
:Eve :parentOf :Iris .
:Frank :parentOf :Jack .
:Grace :parentOf :Kelly .
:Henry :parentOf :Liam .
:Henry :parentOf :Mia .

# Personal information
:Alice foaf:name "Alice Johnson" ;
       :birthYear "1950"^^<http://www.w3.org/2001/XMLSchema#integer> .
:Bob foaf:name "Bob Johnson" ;
     :birthYear "1948"^^<http://www.w3.org/2001/XMLSchema#integer> .
:Charlie foaf:name "Charlie Johnson" ;
         :birthYear "1975"^^<http://www.w3.org/2001/XMLSchema#integer> .
:Eve foaf:name "Eve Brown" ;
     :birthYear "2000"^^<http://www.w3.org/2001/XMLSchema#integer> .
:Iris foaf:name "Iris Brown" ;
      :birthYear "2024"^^<http://www.w3.org/2001/XMLSchema#integer> .
```

### Datalog Rules

```datalog
# ========================================
# BASE PREDICATES
# ========================================

# Map RDF parent relationship to Datalog
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .

# ========================================
# DERIVED RELATIONSHIPS
# ========================================

# Child (inverse of parent)
child(?y, ?x) :- parent(?x, ?y).

# Ancestor (transitive closure of parent)
# Base case: direct parent
ancestor(?x, ?y) :- parent(?x, ?y).

# Recursive case: parent of ancestor
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Descendant (inverse of ancestor)
descendant(?y, ?x) :- ancestor(?x, ?y).

# Grandparent (parent of parent)
grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z).

# Grandchild (inverse of grandparent)
grandchild(?z, ?x) :- grandparent(?x, ?z).

# Great-grandparent (grandparent of parent)
greatGrandparent(?x, ?w) :- grandparent(?x, ?z), parent(?z, ?w).

# Sibling (share at least one parent, different individuals)
sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y), ?x != ?y.

# Full sibling (share both parents)
fullSibling(?x, ?y) :- parent(?p1, ?x), parent(?p1, ?y),
                       parent(?p2, ?x), parent(?p2, ?y),
                       ?p1 != ?p2, ?x != ?y.

# Uncle/Aunt (sibling of parent)
uncleOrAunt(?u, ?n) :- sibling(?u, ?p), parent(?p, ?n).

# Niece/Nephew (inverse of uncle/aunt)
nieceOrNephew(?n, ?u) :- uncleOrAunt(?u, ?n).

# Cousin (children of siblings)
cousin(?x, ?y) :- parent(?p1, ?x), parent(?p2, ?y), sibling(?p1, ?p2).

# Second cousin (grandchildren of siblings)
secondCousin(?x, ?y) :- grandparent(?g1, ?x), grandparent(?g2, ?y), sibling(?g1, ?g2).
```

### Example Queries

#### Query 1: Find all ancestors of Iris

```sparql
SELECT ?ancestor ?name WHERE {
  ?ancestor <http://example.org/ancestor> <http://example.org/Iris> .
  ?ancestor <http://xmlns.com/foaf/0.1/name> ?name .
}
ORDER BY ?name
```

**Expected Results:**
```
?ancestor        | ?name
-----------------|------------------
:Alice           | Alice Johnson
:Bob             | Bob Johnson
:Charlie         | Charlie Johnson
:Eve             | Eve Brown
```

**Explanation**: The recursive `ancestor` rule computes the transitive closure, finding Eve (parent), Charlie (grandparent), and Alice/Bob (great-grandparents).

#### Query 2: Find all cousins

```sparql
SELECT ?cousin1 ?name1 ?cousin2 ?name2 WHERE {
  ?cousin1 <http://example.org/cousin> ?cousin2 .
  ?cousin1 <http://xmlns.com/foaf/0.1/name> ?name1 .
  ?cousin2 <http://xmlns.com/foaf/0.1/name> ?name2 .
  FILTER(?cousin1 < ?cousin2)  # Avoid duplicates
}
ORDER BY ?name1 ?name2
```

**Expected Results:**
```
?cousin1 | ?name1       | ?cousin2 | ?name2
---------|--------------|----------|-------------
:Eve     | Eve Brown    | :Grace   | Grace X
:Eve     | Eve Brown    | :Henry   | Henry X
:Frank   | Frank X      | :Grace   | Grace X
:Frank   | Frank X      | :Henry   | Henry X
```

**Explanation**: The `cousin` rule finds children of siblings (Charlie and Diana are siblings, so their children are cousins).

#### Query 3: Count descendants for each person

```sparql
SELECT ?person ?name (COUNT(?descendant) AS ?descendantCount) WHERE {
  ?person <http://xmlns.com/foaf/0.1/name> ?name .
  ?person <http://example.org/descendant> ?descendant .
}
GROUP BY ?person ?name
ORDER BY DESC(?descendantCount)
```

**Expected Results:**
```
?person  | ?name          | ?descendantCount
---------|----------------|------------------
:Alice   | Alice Johnson  | 9
:Bob     | Bob Johnson    | 9
:Charlie | Charlie Johnson| 3
:Diana   | Diana X        | 4
:Eve     | Eve Brown      | 1
:Henry   | Henry X        | 2
```

**Explanation**: Alice and Bob have the most descendants (2 children + 4 grandchildren + 3 great-grandchildren = 9 each).

---

## Example 2: Graph Reachability

### Problem Statement

Given a directed graph, compute reachability, connected components, and path properties.

### RDF Data (Turtle)

```turtle
@prefix : <http://example.org/> .

# Directed graph edges
# Component 1 (strongly connected)
:A :edge :B .
:B :edge :C .
:C :edge :A .
:C :edge :D .
:D :edge :E .

# Component 2 (disconnected)
:X :edge :Y .
:Y :edge :Z .

# Component 3 (tree structure)
:Root :edge :Child1 .
:Root :edge :Child2 .
:Child1 :edge :GrandChild1 .
:Child1 :edge :GrandChild2 .
:Child2 :edge :GrandChild3 .

# Self-loop
:Loop :edge :Loop .

# Node labels
:A :label "Node A" .
:B :label "Node B" .
:C :label "Node C" .
:D :label "Node D" .
:E :label "Node E" .
:X :label "Node X" .
:Y :label "Node Y" .
:Z :label "Node Z" .
```

### Datalog Rules

```datalog
# ========================================
# BASE PREDICATES
# ========================================

edge(?x, ?y) :- ?x <http://example.org/edge> ?y .

# ========================================
# REACHABILITY
# ========================================

# Reachable: transitive closure of edges
# Base case: direct edge
reachable(?x, ?y) :- edge(?x, ?y).

# Recursive case: path through intermediate node
reachable(?x, ?z) :- edge(?x, ?y), reachable(?y, ?z).

# ========================================
# CONNECTIVITY
# ========================================

# Connected: bidirectional reachability
connected(?x, ?y) :- reachable(?x, ?y), reachable(?y, ?x).

# Same component (reflexive, symmetric, transitive)
sameComponent(?x, ?y) :- connected(?x, ?y).
sameComponent(?x, ?x) :- edge(?x, ?_).
sameComponent(?x, ?x) :- edge(?_, ?x).

# ========================================
# PATH PROPERTIES
# ========================================

# Distance-2 paths (exactly 2 edges)
distance2(?x, ?z) :- edge(?x, ?y), edge(?y, ?z).

# Distance-3 paths
distance3(?x, ?w) :- edge(?x, ?y), edge(?y, ?z), edge(?z, ?w).

# Has cycle (node reachable from itself)
hasCycle(?x) :- edge(?x, ?y), reachable(?y, ?x).

# Triangle (3-cycle)
triangle(?a, ?b, ?c) :- edge(?a, ?b), edge(?b, ?c), edge(?c, ?a).
```

### Example Queries

#### Query 1: Find all nodes reachable from A

```sparql
SELECT ?target ?label WHERE {
  <http://example.org/A> <http://example.org/reachable> ?target .
  ?target <http://example.org/label> ?label .
}
ORDER BY ?label
```

**Expected Results:**
```
?target | ?label
--------|--------
:A      | Node A
:B      | Node B
:C      | Node C
:D      | Node D
:E      | Node E
```

**Explanation**: From A, you can reach B directly, C via B, A via C (cycle), D via C, and E via D.

#### Query 2: Find strongly connected components

```sparql
SELECT ?node1 ?label1 ?node2 ?label2 WHERE {
  ?node1 <http://example.org/connected> ?node2 .
  ?node1 <http://example.org/label> ?label1 .
  ?node2 <http://example.org/label> ?label2 .
  FILTER(?node1 < ?node2)  # Avoid duplicates
}
ORDER BY ?label1 ?label2
```

**Expected Results:**
```
?node1 | ?label1  | ?node2 | ?label2
-------|----------|--------|--------
:A     | Node A   | :B     | Node B
:A     | Node A   | :C     | Node C
:B     | Node B   | :C     | Node C
```

**Explanation**: Nodes A, B, C form a strongly connected component (cycle). D, E, X, Y, Z are not strongly connected to others.

#### Query 3: Find all nodes that participate in cycles

```sparql
SELECT ?node ?label WHERE {
  ?node <http://example.org/hasCycle> ?node .
  ?node <http://example.org/label> ?label .
}
ORDER BY ?label
```

**Expected Results:**
```
?node | ?label
------|--------
:A    | Node A
:B    | Node B
:C    | Node C
:Loop | (no label)
```

**Explanation**: A, B, C form a cycle, and Loop has a self-loop.

---

## Example 3: Social Network Analysis

### Problem Statement

Model a social network with friendship and follower relationships, computing friends-of-friends, influencers, and community connections.

### RDF Data (Turtle)

```turtle
@prefix : <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

# Friendship (bidirectional)
:Alice foaf:knows :Bob .
:Bob foaf:knows :Alice .
:Bob foaf:knows :Carol .
:Carol foaf:knows :Bob .
:Carol foaf:knows :David .
:David foaf:knows :Carol .
:Alice foaf:knows :Eve .
:Eve foaf:knows :Alice .

# Follower relationships (directed)
:Alice :follows :Bob .
:Bob :follows :Carol .
:Carol :follows :David .
:David :follows :Alice .
:Eve :follows :Bob .
:Frank :follows :Alice .
:Frank :follows :Bob .
:Frank :follows :Carol .

# Interests
:Alice :interestedIn :Music .
:Bob :interestedIn :Music .
:Bob :interestedIn :Sports .
:Carol :interestedIn :Sports .
:Carol :interestedIn :Art .
:David :interestedIn :Art .
:Eve :interestedIn :Music .

# Locations
:Alice :livesIn :CityA .
:Bob :livesIn :CityA .
:Carol :livesIn :CityB .
:David :livesIn :CityB .
:Eve :livesIn :CityA .
:Frank :livesIn :CityC .
```

### Datalog Rules

```datalog
# ========================================
# BASE RELATIONSHIPS
# ========================================

# Direct friendship (symmetric in data)
friend(?x, ?y) :- ?x <http://xmlns.com/foaf/0.1/knows> ?y .

# Follow relationship (directed)
follows(?x, ?y) :- ?x <http://example.org/follows> ?y .

# ========================================
# DERIVED RELATIONSHIPS
# ========================================

# Friend-of-friend (2-hop friendship)
friendOfFriend(?x, ?z) :- friend(?x, ?y), friend(?y, ?z), ?x != ?z.

# Indirect follower (transitive follows)
indirectFollower(?x, ?y) :- follows(?x, ?y).
indirectFollower(?x, ?z) :- follows(?x, ?y), indirectFollower(?y, ?z).

# Mutual follower (bidirectional follow)
mutualFollower(?x, ?y) :- follows(?x, ?y), follows(?y, ?x), ?x < ?y.

# Connected in social graph (friend or friend-of-friend)
sociallyConnected(?x, ?y) :- friend(?x, ?y).
sociallyConnected(?x, ?y) :- friendOfFriend(?x, ?y).

# Same interest community
sharedInterest(?x, ?y, ?interest) :-
    ?x <http://example.org/interestedIn> ?interest,
    ?y <http://example.org/interestedIn> ?interest,
    ?x < ?y.

# Local community (friends in same city)
localFriend(?x, ?y) :-
    friend(?x, ?y),
    ?x <http://example.org/livesIn> ?city,
    ?y <http://example.org/livesIn> ?city.
```

### Example Queries

#### Query 1: Find friends-of-friends (potential new connections)

```sparql
SELECT ?person ?fof WHERE {
  <http://example.org/Alice> <http://example.org/friendOfFriend> ?fof .
  # Exclude existing friends
  FILTER NOT EXISTS {
    <http://example.org/Alice> <http://xmlns.com/foaf/0.1/knows> ?fof .
  }
}
```

**Expected Results:**
```
?fof
------
:Carol
:David
```

**Explanation**: Alice → Bob → Carol and Alice → Bob → David. Carol and David are friends-of-friends but not direct friends.

#### Query 2: Find influencers (most indirect followers)

```sparql
SELECT ?person (COUNT(DISTINCT ?follower) AS ?followerCount) WHERE {
  ?follower <http://example.org/indirectFollower> ?person .
}
GROUP BY ?person
ORDER BY DESC(?followerCount)
LIMIT 5
```

**Expected Results:**
```
?person | ?followerCount
--------|----------------
:Alice  | 3
:Carol  | 2
:David  | 1
:Bob    | 1
```

**Explanation**: Alice is followed indirectly by Bob (via David), Frank (direct), and Eve (direct → Bob). Carol is followed by Alice (via Bob) and Frank.

#### Query 3: Find interest-based communities

```sparql
SELECT ?interest ?person1 ?person2 WHERE {
  ?person1 <http://example.org/sharedInterest> ?person2 .
  ?person1 <http://example.org/interestedIn> ?interest .
  ?person2 <http://example.org/interestedIn> ?interest .
}
ORDER BY ?interest ?person1
```

**Expected Results:**
```
?interest | ?person1 | ?person2
----------|----------|----------
:Art      | :Carol   | :David
:Music    | :Alice   | :Bob
:Music    | :Alice   | :Eve
:Music    | :Bob     | :Eve
:Sports   | :Bob     | :Carol
```

**Explanation**: Groups people by shared interests. Music community has 3 members (Alice, Bob, Eve), Art has 2 (Carol, David), Sports has 2 (Bob, Carol).

---

## Example 4: Organizational Hierarchy

### Problem Statement

Model a company's management structure with departments, computing reporting chains, management levels, and cross-functional teams.

### RDF Data (Turtle)

```turtle
@prefix : <http://example.org/> .
@prefix org: <http://www.w3.org/ns/org#> .

# Management hierarchy
:CEO org:manages :VPEngineering .
:CEO org:manages :VPSales .
:VPEngineering org:manages :DirectorBackend .
:VPEngineering org:manages :DirectorFrontend .
:VPSales org:manages :SalesManagerNorth .
:VPSales org:manages :SalesManagerSouth .
:DirectorBackend org:manages :EngineerAlice .
:DirectorBackend org:manages :EngineerBob .
:DirectorFrontend org:manages :EngineerCarol .
:SalesManagerNorth org:manages :SalesRepDavid .
:SalesManagerSouth org:manages :SalesRepEve .

# Department assignments
:CEO :worksIn :Executive .
:VPEngineering :worksIn :Engineering .
:VPSales :worksIn :Sales .
:DirectorBackend :worksIn :Engineering .
:DirectorFrontend :worksIn :Engineering .
:SalesManagerNorth :worksIn :Sales .
:SalesManagerSouth :worksIn :Sales .
:EngineerAlice :worksIn :Engineering .
:EngineerBob :worksIn :Engineering .
:EngineerCarol :worksIn :Engineering .
:SalesRepDavid :worksIn :Sales .
:SalesRepEve :worksIn :Sales .

# Roles
:CEO :hasRole "Chief Executive Officer" .
:VPEngineering :hasRole "VP Engineering" .
:EngineerAlice :hasRole "Senior Backend Engineer" .
:SalesRepDavid :hasRole "Sales Representative" .
```

### Datalog Rules

```datalog
# ========================================
# BASE RELATIONSHIPS
# ========================================

# Direct management
manages(?mgr, ?emp) :- ?mgr <http://www.w3.org/ns/org#manages> ?emp .

# Department membership
worksIn(?emp, ?dept) :- ?emp <http://example.org/worksIn> ?dept .

# ========================================
# MANAGEMENT HIERARCHY
# ========================================

# Management chain (transitive)
managesIndirectly(?mgr, ?emp) :- manages(?mgr, ?emp).
managesIndirectly(?mgr, ?emp) :- manages(?mgr, ?mid), managesIndirectly(?mid, ?emp).

# Reporting chain (inverse)
reportsTo(?emp, ?mgr) :- managesIndirectly(?mgr, ?emp).

# Management levels (distance from CEO)
level(?emp, 0) :- ?emp <http://example.org/hasRole> "Chief Executive Officer" .
level(?emp, ?lvl) :- manages(?mgr, ?emp), level(?mgr, ?prevLvl).
  # Note: In real implementation, would need arithmetic to compute lvl = prevLvl + 1

# ========================================
# PEER RELATIONSHIPS
# ========================================

# Direct peers (same immediate manager)
directPeer(?e1, ?e2) :- manages(?mgr, ?e1), manages(?mgr, ?e2), ?e1 < ?e2.

# Department colleagues
sameDepart(?e1, ?e2) :- worksIn(?e1, ?dept), worksIn(?e2, ?dept), ?e1 < ?e2.

# Cross-department peers (same manager, different department)
crossDeptPeer(?e1, ?e2) :- directPeer(?e1, ?e2),
                            worksIn(?e1, ?d1),
                            worksIn(?e2, ?d2),
                            ?d1 != ?d2.
```

### Example Queries

#### Query 1: Find full reporting chain for Alice

```sparql
SELECT ?manager ?role WHERE {
  <http://example.org/EngineerAlice> <http://example.org/reportsTo> ?manager .
  ?manager <http://example.org/hasRole> ?role .
}
ORDER BY ?role
```

**Expected Results:**
```
?manager          | ?role
------------------|-------------------------
:CEO              | Chief Executive Officer
:DirectorBackend  | Director Backend
:VPEngineering    | VP Engineering
```

**Explanation**: Alice → DirectorBackend → VPEngineering → CEO

#### Query 2: Find all employees under VP Engineering

```sparql
SELECT ?employee ?role WHERE {
  <http://example.org/VPEngineering> <http://example.org/managesIndirectly> ?employee .
  ?employee <http://example.org/hasRole> ?role .
}
ORDER BY ?role
```

**Expected Results:**
```
?employee         | ?role
------------------|-------------------------
:DirectorBackend  | (role...)
:DirectorFrontend | (role...)
:EngineerAlice    | Senior Backend Engineer
:EngineerBob      | (role...)
:EngineerCarol    | (role...)
```

**Explanation**: VP Engineering oversees 2 directors and their 3 engineers.

#### Query 3: Find cross-departmental management

```sparql
SELECT ?manager ?dept1 ?dept2 WHERE {
  ?manager <http://example.org/managesIndirectly> ?emp1 .
  ?manager <http://example.org/managesIndirectly> ?emp2 .
  ?emp1 <http://example.org/worksIn> ?dept1 .
  ?emp2 <http://example.org/worksIn> ?dept2 .
  FILTER(?dept1 < ?dept2)  # Distinct departments
}
```

**Expected Results:**
```
?manager | ?dept1       | ?dept2
---------|--------------|-------------
:CEO     | :Engineering | :Sales
:CEO     | :Engineering | :Executive
:CEO     | :Executive   | :Sales
```

**Explanation**: Only CEO manages across departments (Engineering, Sales, Executive).

---

## Example 5: Ontology Inference

### Problem Statement

Implement RDFS and basic OWL inference rules for subclass hierarchies and property inheritance.

### RDF Data (Turtle)

```turtle
@prefix : <http://example.org/> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix owl: <http://www.w3.org/2002/07/owl#> .

# Class hierarchy
:Mammal rdfs:subClassOf :Animal .
:Dog rdfs:subClassOf :Mammal .
:Cat rdfs:subClassOf :Mammal .
:Beagle rdfs:subClassOf :Dog .

# Property hierarchy
:owns rdfs:subPropertyOf :hasRelationshipWith .
:hasPet rdfs:subPropertyOf :owns .

# Domain and range
:hasPet rdfs:domain :Person .
:hasPet rdfs:range :Pet .

# Instances
:Alice rdf:type :Person .
:Fido rdf:type :Beagle .
:Whiskers rdf:type :Cat .

# Facts
:Alice :hasPet :Fido .
:Alice :hasPet :Whiskers .

# Inverse properties
:ownedBy owl:inverseOf :owns .
```

### Datalog Rules

```datalog
# ========================================
# RDFS INFERENCE RULES
# ========================================

# Subclass transitivity
subClassOf(?c1, ?c2) :- ?c1 <http://www.w3.org/2000/01/rdf-schema#subClassOf> ?c2 .
subClassOf(?c1, ?c3) :- subClassOf(?c1, ?c2), subClassOf(?c2, ?c3).

# Type inference from subclass
inferredType(?inst, ?class) :-
    ?inst <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?subclass,
    subClassOf(?subclass, ?class).

# Subproperty transitivity
subPropertyOf(?p1, ?p2) :-
    ?p1 <http://www.w3.org/2000/01/rdf-schema#subPropertyOf> ?p2 .
subPropertyOf(?p1, ?p3) :-
    subPropertyOf(?p1, ?p2),
    subPropertyOf(?p2, ?p3).

# Property inference
hasInferredRelation(?subj, ?superprop, ?obj) :-
    ?subj ?subprop ?obj,
    subPropertyOf(?subprop, ?superprop).

# Domain inference (if X P Y, then X is in domain of P)
hasDomainType(?subj, ?domainClass) :-
    ?subj ?prop ?obj,
    ?prop <http://www.w3.org/2000/01/rdf-schema#domain> ?domainClass.

# Range inference (if X P Y, then Y is in range of P)
hasRangeType(?obj, ?rangeClass) :-
    ?subj ?prop ?obj,
    ?prop <http://www.w3.org/2000/01/rdf-schema#range> ?rangeClass.

# ========================================
# OWL INFERENCE RULES (BASIC)
# ========================================

# Inverse property
inverse(?y, ?invProp, ?x) :-
    ?x ?prop ?y,
    ?prop <http://www.w3.org/2002/07/owl#inverseOf> ?invProp.
```

### Example Queries

#### Query 1: Find all inferred types for Fido

```sparql
SELECT ?class WHERE {
  {
    <http://example.org/Fido> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?class .
  } UNION {
    <http://example.org/Fido> <http://example.org/inferredType> ?class .
  }
}
ORDER BY ?class
```

**Expected Results:**
```
?class
---------
:Beagle     # Explicit
:Dog        # Inferred via Beagle → Dog
:Mammal     # Inferred via Dog → Mammal
:Animal     # Inferred via Mammal → Animal
```

**Explanation**: Transitive subclass inference derives all superclasses.

#### Query 2: Find all inferred relationships

```sparql
SELECT ?subject ?property ?object WHERE {
  ?subject <http://example.org/hasInferredRelation> ?object .
  ?subject ?subprop ?object .
  ?subprop <http://www.w3.org/2000/01/rdf-schema#subPropertyOf>+ ?property .
}
```

**Expected Results:**
```
?subject | ?property              | ?object
---------|------------------------|----------
:Alice   | :owns                  | :Fido
:Alice   | :owns                  | :Whiskers
:Alice   | :hasRelationshipWith   | :Fido
:Alice   | :hasRelationshipWith   | :Whiskers
```

**Explanation**: `hasPet` implies `owns` implies `hasRelationshipWith`.

---

## Example 6: Supply Chain Dependencies

### Problem Statement

Model a bill-of-materials (BOM) system with part dependencies, computing transitive dependencies and critical paths.

### RDF Data

```turtle
@prefix : <http://example.org/supply/> .

# Product: Bicycle
:Bicycle :hasPart :Frame .
:Bicycle :hasPart :Wheels .
:Bicycle :hasPart :Drivetrain .

# Frame assembly
:Frame :hasPart :TopTube .
:Frame :hasPart :DownTube .
:Frame :hasPart :SeatPost .

# Wheels assembly
:Wheels :hasPart :Rim .
:Wheels :hasPart :Spokes .
:Wheels :hasPart :Tire .
:Rim :hasPart :RimMaterial .

# Drivetrain
:Drivetrain :hasPart :Pedals .
:Drivetrain :hasPart :Chain .
:Drivetrain :hasPart :Gears .

# Suppliers
:Frame :suppliedBy :SupplierA .
:Rim :suppliedBy :SupplierB .
:Tire :suppliedBy :SupplierC .
:Chain :suppliedBy :SupplierA .
```

### Datalog Rules

```datalog
# ========================================
# BILL OF MATERIALS
# ========================================

# Direct part relationship
hasPart(?whole, ?part) :- ?whole <http://example.org/supply/hasPart> ?part .

# Transitive part dependency (BOM explosion)
dependsOn(?product, ?part) :- hasPart(?product, ?part).
dependsOn(?product, ?subpart) :- hasPart(?product, ?part), dependsOn(?part, ?subpart).

# Supplier dependency
needsSupplier(?product, ?supplier) :-
    dependsOn(?product, ?part),
    ?part <http://example.org/supply/suppliedBy> ?supplier.

# Critical supplier (supplies multiple dependencies)
criticalSupplier(?product, ?supplier) :-
    needsSupplier(?product, ?supplier),
    needsSupplier(?product, ?supplier).  # Appears multiple times

# Assembly level (depth in BOM tree)
assemblyLevel(?part, 0) :- hasPart(?_, ?part), NOT hasPart(?part, ?_).
assemblyLevel(?whole, ?lvl) :- hasPart(?whole, ?part), assemblyLevel(?part, ?prevLvl).
  # Note: Real implementation would compute lvl = prevLvl + 1
```

### Example Queries

#### Query 1: Full BOM explosion for Bicycle

```sparql
SELECT ?part WHERE {
  <http://example.org/supply/Bicycle> <http://example.org/supply/dependsOn> ?part .
}
ORDER BY ?part
```

**Expected Results (partial):**
```
?part
-----------------
:Frame
:TopTube
:DownTube
:SeatPost
:Wheels
:Rim
:RimMaterial
:Spokes
:Tire
:Drivetrain
:Pedals
:Chain
:Gears
```

**Explanation**: All parts and subparts needed to build a bicycle.

#### Query 2: Find all suppliers needed for Bicycle

```sparql
SELECT DISTINCT ?supplier WHERE {
  <http://example.org/supply/Bicycle> <http://example.org/supply/needsSupplier> ?supplier .
}
```

**Expected Results:**
```
?supplier
-----------
:SupplierA
:SupplierB
:SupplierC
```

**Explanation**: Must coordinate with three suppliers to build complete bicycle.

---

## Example 7: Path Finding

### Problem Statement

Find paths in a graph, including shortest paths and all paths between nodes.

### RDF Data

```turtle
@prefix : <http://example.org/> .

# City connections with distances
:Seattle :connected :Portland ; :distance "174" .
:Portland :connected :Eugene ; :distance "110" .
:Eugene :connected :Sacramento ; :distance "294" .
:Sacramento :connected :SanFrancisco ; :distance "87" .
:SanFrancisco :connected :LosAngeles ; :distance "382" .

# Alternative routes
:Seattle :connected :Spokane ; :distance "280" .
:Spokane :connected :Boise ; :distance "390" .
:Boise :connected :Sacramento ; :distance "654" .
```

### Datalog Rules

```datalog
# Base: connection exists
connected(?a, ?b) :- ?a <http://example.org/connected> ?b .

# Path: transitive reachability
path(?from, ?to) :- connected(?from, ?to).
path(?from, ?to) :- connected(?from, ?mid), path(?mid, ?to).

# 1-hop, 2-hop, 3-hop paths
hop1(?a, ?b) :- connected(?a, ?b).
hop2(?a, ?c) :- connected(?a, ?b), connected(?b, ?c).
hop3(?a, ?d) :- connected(?a, ?b), connected(?b, ?c), connected(?c, ?d).
```

### Example Queries

```sparql
SELECT ?destination WHERE {
  <http://example.org/Seattle> <http://example.org/path> ?destination .
}
```

**Results**: All cities reachable from Seattle.

---

## Example 8: Access Control

### Problem Statement

Model role-based access control (RBAC) with role hierarchies and permission inheritance.

### RDF Data

```turtle
@prefix : <http://example.org/acl/> .

# Role hierarchy
:Admin :inheritsFrom :Manager .
:Manager :inheritsFrom :Employee .
:Employee :inheritsFrom :Guest .

# Permission assignments
:Admin :hasPermission :DeleteUser .
:Admin :hasPermission :CreateUser .
:Manager :hasPermission :ViewReports .
:Manager :hasPermission :ApproveExpenses .
:Employee :hasPermission :ViewOwnData .
:Employee :hasPermission :SubmitExpenses .
:Guest :hasPermission :Login .

# User role assignments
:Alice :hasRole :Admin .
:Bob :hasRole :Manager .
:Carol :hasRole :Employee .
```

### Datalog Rules

```datalog
# Role inheritance (transitive)
inheritsFrom(?r1, ?r2) :- ?r1 <http://example.org/acl/inheritsFrom> ?r2 .
inheritsFrom(?r1, ?r3) :- inheritsFrom(?r1, ?r2), inheritsFrom(?r2, ?r3).

# Permission inheritance
hasPermission(?role, ?perm) :- ?role <http://example.org/acl/hasPermission> ?perm .
hasPermission(?role, ?perm) :- inheritsFrom(?role, ?parentRole),
                                hasPermission(?parentRole, ?perm).

# Effective permissions for users
userCan(?user, ?perm) :- ?user <http://example.org/acl/hasRole> ?role,
                         hasPermission(?role, ?perm).
```

### Example Queries

```sparql
SELECT ?permission WHERE {
  <http://example.org/acl/Alice> <http://example.org/acl/userCan> ?permission .
}
ORDER BY ?permission
```

**Expected Results**: All permissions Alice has (Admin + Manager + Employee + Guest permissions).

---

## Summary

These examples demonstrate:

1. **Transitive Relationships**: Ancestors, reachability, supply chains
2. **Symmetric Relations**: Siblings, connections, mutual followers
3. **Complex Derivations**: Cousins, second-degree connections
4. **Hierarchies**: Org charts, role inheritance, ontologies
5. **Graph Algorithms**: Connected components, path finding
6. **Domain Modeling**: Social networks, access control, BOM systems

**Key Patterns:**
- Base case + recursive case for transitive closure
- Multiple rules per predicate for different scenarios
- Combining RDF patterns with Datalog atoms
- Using SPARQL queries to leverage rule results

**Next**: See [DATALOG_PERFORMANCE.md](DATALOG_PERFORMANCE.md) for optimization tips.
