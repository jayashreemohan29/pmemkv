# pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*


Overview
--------
This fork tests pmemkv for crash-consistency. A sample test case is `kvtree-test.c`, which puts a value in the kvtree engine and checks if the recovery is correct, using pmreorder. To run this test file,
	1. ./rebuild.sh (will build pmemkv and install it)
	2. make mytests
	3. ./TEST1.sh ReorderAccumulative 2>&1 | tee out.log
  
* The log file from pmreorder will be stored at pmreorder.log (This file is opened in append mode. So clear it after each run if required)
* I have enabled some logging in pmreorder to track which set of store instructions are being replayed. You could patch these from [here](https://github.com/jayashreemohan29/pmdk-test/commit/87d77a752ad60e6be2fdf88725d07195c8c79ed9)
* Also, compile libpmemobj-c++ with the extra flag LIBPMEMOBJ\_CPP\_VG\_ENABLED=1 (to enable valgrind tracing)

---------

`pmemkv` is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation, `pmemkv`
provides different options for storage engines and language bindings.

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">Installation</a></li>
<li><a href="#engines">Storage Engines</a></li>
<li><a href="#bindings">Language Bindings</a></li>
<li><a href="#tools">Tools and Utilities</a></li>
</ul>

<a name="installation"></a>

Installation
------------

`pmemkv` does not currently provide install packages, but our
<a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">installation</a> guide
provides detailed instructions, including configuring DAX and pool sets. 

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources">Building From Sources</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora">Installing on Fedora</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#experimental">Using Experimental Engines</a></li>
</ul>


<a name="engines"></a>

Storage Engines
---------------

`pmemkv` provides multiple storage engines with vastly different implementations. Since all
engines conform to the same common API, any engine can be used with common `pmemkv` utilities
and language bindings. Engines are requested at runtime by name.
[Contributing a new engine](https://github.com/pmem/pmemkv/blob/master/CONTRIBUTING.md#engines)
is easy and encouraged!

![pmemkv-engines](https://user-images.githubusercontent.com/913363/34419331-68619cfe-ebc0-11e7-9443-fa13dc9decbb.png)

### Available Engines

| Engine  | Description | Experimental? | Concurrent? | Sorted? |
| ------- | ----------- | ------------- | ----------- | ------- |
| [blackhole](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#blackhole) | Accepts everything, returns nothing | No | Yes | No |
| [kvtree3](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#kvtree3) | Hybrid B+ persistent tree | No | No | No |
| [vmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vmap) | Volatile hash map | No | No | Yes |
| [vcmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vcmap) | Volatile concurrent hash map | No | Yes | No |
| btree | Copy-on-write B+ persistent tree | Yes | No | Yes |
| caching | Caching for remote Memcached or Redis server | Yes | Yes | - |

<a name="bindings"></a>

Language Bindings
-----------------

`pmemkv` is written in C and C++. Developers can either use native C++ classes directly, or use our `extern "C"` API,
or use one of several high-level language bindings that are based on the `extern "C"` API.

![pmemkv-bindings](https://user-images.githubusercontent.com/913363/52880816-4651ef00-3120-11e9-9ab4-7eb006b4c7f5.png)

### C++ Example

```cpp
#include <iostream>
#include "libpmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

int main() {
    LOG("Starting engine");
    KVEngine* kv = KVEngine::Start("vmap", "{\"path\":\"/dev/shm/\"}");

    LOG("Putting new key");
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK && kv->Count() == 1);

    LOG("Reading key back");
    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    LOG("Iterating existing keys");
    kv->Put("key2", "value2");
    kv->Put("key3", "value3");
    kv->All([](const string& k) {
        LOG("  visited: " << k);
    });

    LOG("Removing existing key");
    s = kv->Remove("key1");
    assert(s == OK && !kv->Exists("key1"));

    LOG("Stopping engine");
    delete kv;
    return 0;
}
```

### C Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "libpmemkv.h"

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

void StartFailureCallback(void* context, const char* engine, const char* config, const char* msg) {
    printf("ERROR: %s\n", msg);
    exit(-1);
}

void AllCallback(void* context, int kb, const char* k) {
    printf("   visited: %s\n", k);
}

int main() {
    LOG("Starting engine");
    KVEngine* kv = kvengine_start(NULL, "vmap", "{\"path\":\"/dev/shm/\"}", &StartFailureCallback);

    LOG("Putting new key");
    char* key1 = "key1";
    char* value1 = "value1";
    KVStatus s = kvengine_put(kv, strlen(key1), key1, strlen(value1), value1);
    assert(s == OK && kvengine_count(kv) == 1);

    LOG("Reading key back");
    char val[MAX_VAL_LEN];
    s = kvengine_get_copy(kv, strlen(key1), key1, MAX_VAL_LEN, val);
    assert(s == OK && !strcmp(val, "value1"));

    LOG("Iterating existing keys");
    char* key2 = "key2";
    char* value2 = "value2";
    char* key3 = "key3";
    char* value3 = "value3";
    kvengine_put(kv, strlen(key2), key2, strlen(value2), value2);
    kvengine_put(kv, strlen(key3), key3, strlen(value3), value3);
    kvengine_all(kv, NULL, &AllCallback);

    LOG("Removing existing key");
    s = kvengine_remove(kv, strlen(key1), key1);
    assert(s == OK && kvengine_exists(kv, strlen(key1), key1) == NOT_FOUND);

    LOG("Stopping engine");
    kvengine_stop(kv);
    return 0;
}
```

### Other Languages

These bindings are maintained in separate GitHub repos, but are still kept
in sync with the main `pmemkv` distribution.
 
* Java - https://github.com/pmem/pmemkv-java
* Node.js - https://github.com/pmem/pmemkv-nodejs
* Ruby - https://github.com/pmem/pmemkv-ruby

<a name="tools"></a>

Tools and Utilities
-------------------

Benchmarks, examples and other helpful utilities are available here:

https://github.com/pmem/pmemkv-tools
