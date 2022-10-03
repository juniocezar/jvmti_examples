
## JVM agent for inspecting methods being dynamically called

## Software used

### Java version: 19
    * https://www.oracle.com/java/technologies/downloads/
    * https://download.oracle.com/java/19/latest/jdk-19_linux-x64_bin.deb 
    * export JAVA_HOME=/usr/lib/jvm/jdk-19

### GCC version: 11.2.0

### Based on
https://gite.lirmm.fr/quintaoper/Jinn/-/tree/master/src/sync_jvmti

## Compiling

```sh
$ export JAVA_HOME=/usr/lib/jvm/jdk-19
$ make

mkdir -p bin
g++ -std=c++14 -O2 -pedantic -Wall -Wextra -Wno-unused-parameter -I /usr/lib/jvm/jdk-19/include -I /usr/lib/jvm/jdk-19/include/linux src/agent.cpp -shared -fpic -o bin/sync_jvmti.so

cd java; /usr/lib/jvm/jdk-19/bin/javac HelloWorld.java

```


## Running

```sh

$  ./run.sh -cp java/ HelloWorld  | grep HelloWorld -A 5
sync_jvmti: Agent has been loaded.

Signature(ClassName): 'LHelloWorld;'
	 Method: 'main'
		param_size: 1

Signature(ClassName): 'Ljava/lang/ClassLoader;'
	 Method: 'loadClass'
--
Signature(ClassName): 'LHelloWorld;'
	 Method: 'megaFoo'
		param_size: 3
		slot:0, start:0, cur:140009650325200, param:Ljava/lang/String; inStr
		slot:1, start:0, cur:140009650325200, param:Ljava/lang/Integer; inInt
		slot:2, start:0, cur:140009650325200, param:Ljava/lang/Boolean; inBool
--
Signature(ClassName): 'LHelloWorld;'
	 Method: 'superFoo'
		param_size: 0

Signature(ClassName): 'Ljava/io/PrintStream;'
	 Method: 'println'

sync_jvmti: Agent has been unloaded
```