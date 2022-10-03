#include <jvmti.h>
#include <cassert>
#include <cstring>
#include <atomic>
#include <algorithm>
#include <string>
using std::memory_order_relaxed;

// used some examples from:
// https://github.com/frohoff/jdk8u-dev-jdk/blob/master/src/share/demo/jvmti/compiledMethodLoad/compiledMethodLoad.c
// https://github.com/openjdk/jdk19/blob/59b0de6bc7064b39cdc51517dee4f4d96af3efaf/test/hotspot/jtreg/vmTestbase/nsk/jvmti/GetMethodName/methname002/methname002.cpp
// https://github.com/jon-bell/bytecode-examples/blob/master/heapTagging/src/main/c/tagger.cpp
// https://github.com/dacapobench/dacapobench/blob/master/benchmarks/agent/c/src/dacapomonitor.c
// https://stackoverflow.com/questions/2882150/how-to-get-parameter-values-in-a-methodentry-callback

//
// checks if the input errnum matches any known jvmti error code
static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum,
		const char *str) {
	if (errnum != JVMTI_ERROR_NONE) {
		char *errnum_str;

		errnum_str = NULL;
		(void) jvmti->GetErrorName(errnum, &errnum_str);

		printf("ERROR: JVMTI: %d(%s): %s\n", errnum,
				(errnum_str == NULL ? "Unknown" : errnum_str),
				(str == NULL ? "" : str));
	}
}

//
// auxiliary function for printing the parameters of a java method
static void print_parameters(jvmtiEnv *m_jvmti,
            JNIEnv* jni,
            jthread thread, 
            jmethodID method) {
    jvmtiError error;
    jint param_size = 0;

    error = m_jvmti->GetArgumentsSize(method, &param_size);
    printf("\t\tparam_size: %d\n", param_size);             

    // visit local variable
    jint entry_count = 0;
    jvmtiLocalVariableEntry *table_ptr = NULL;      
    jlocation cur_loc;

    // this call may return JVMTI_ERROR_ABSENT_INFORMATION if the LocalVariableTable
    // is not present. For avoiding JVMTI_ERROR_ABSENT_INFORMATION, you can compile
    // the java code with the '-g' flag
    error = m_jvmti->GetLocalVariableTable(method, &entry_count, &table_ptr);
    check_jvmti_error(m_jvmti, error, "Failed to load LocalVariableTable");

    m_jvmti->GetFrameLocation(thread, 0, NULL, &cur_loc);
    for(int j=0; j<std::min(param_size, entry_count); j++) {
        if(table_ptr[j].start_location > cur_loc) break;
        if(table_ptr[j].signature[0] == 'L') { //   fully-qualified-class
            // the value can be retrieved with the methods listed at:
            // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#local
            // ex: m_jvmti->GetLocalObject(thread, 0, table_ptr[j].slot, jobject &param_obj);
            // ex: m_jvmti->GetLocalInt(thread, 0, table_ptr[j].slot, jint &param_obj);
            printf("\t\tslot:%d, start:%ld, cur:%ld, param:%s %s\n", table_ptr[j].slot,
                table_ptr[j].start_location, cur_loc, table_ptr[j].signature, table_ptr[j].name);

            m_jvmti->Deallocate(reinterpret_cast<unsigned char*>(table_ptr[j].signature));
            m_jvmti->Deallocate(reinterpret_cast<unsigned char*>(table_ptr[j].name));
            m_jvmti->Deallocate(reinterpret_cast<unsigned char*>(table_ptr[j].generic_signature));
        }

    } 
    error = m_jvmti->Deallocate(reinterpret_cast<unsigned char*>(table_ptr));
    check_jvmti_error(m_jvmti, error, "Deallocating resources");
}

// MethodEntryCallBack is a callback function called for
// every method accessed by the JVM
static void JNICALL
MethodEntryCallBack(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env, // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#JNIEnv
            jthread thread, // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#jthread
            jmethodID method) { // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#jmethodID
    jvmtiError error;
    jclass clazz;
    char* name;
    char* signature;

    // get declaring class of the method
    error = jvmti_env->GetMethodDeclaringClass(method, &clazz); 
    check_jvmti_error(jvmti_env, error, "Cannot load declaring class");
    // get the signature of the class
    error = jvmti_env->GetClassSignature(clazz, &signature, 0);
    check_jvmti_error(jvmti_env, error, "Cannot load class signature");
    // get method name
    error = jvmti_env->GetMethodName(method, &name, NULL, NULL);
    check_jvmti_error(jvmti_env, error, "Cannot load method name");

    printf("\nSignature(ClassName): '%s'\n\t Method: '%s'\n", signature, name);
    print_parameters(jvmti_env, jni_env, thread, method);
}

/// Called once the VM is ready.
static void JNICALL
VMInit(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread) {
    printf("JVM is ready");
}

/// Called once the VM shutdowns.
static void JNICALL
VMDeath(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env) {
    printf("JVM finished the code execution");
}

/// Called by the virtual machine to configure the agent.
JNIEXPORT jint JNICALL 
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    jvmtiError err;
    jvmtiEnv *jvmti;
    //
    // Get the JVMTI environment. check required to make sure
    // we are able to properly instrument the code
    jint res = vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_0);
    if (res != JNI_OK || jvmti == NULL) {
        fprintf(stderr, "sync_jvmti: Failed to get environment\n");
        return 1;
    }


    //
    // Initialize the capabilities
    // Reference list of capabilities present at:
    // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#jvmtiCapabilities
    //
    // Note: Please pay attention to the Event Enabling Capabilities table, as it describes,
    // for each capability, the events we can define callback functions later
    //
    // As we are interested in intercepting methods call, we will enable
    // the can_generate_method_entry_events capability. Events: MethodEntry
    // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#jvmtiCapabilities.can_generate_method_entry_events
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_access_local_variables	 = true; // required for reading the method parameters
    caps.can_generate_method_entry_events = true;
    if ((err = jvmti->AddCapabilities(&caps))) {
        fprintf(stderr, "sync_jvmti: Failed to add capabilities (%d)\n", err);
        return 1;
    }


    //
    // Bind callback methods for each event type we previously
    // enabled through the capabilities flags
    // Details at: https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#EventIndex
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    //
    // For each event, we have examples on how to enable them.
    // For MethodEntry, the example is present at:
    // https://docs.oracle.com/en/java/javase/19/docs/specs/jvmti.html#MethodEntry
    // under the Enabling column of the first table
    callbacks.MethodEntry = MethodEntryCallBack;
    jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                    JVMTI_EVENT_METHOD_ENTRY, // Event Type
                                    NULL);
    //
    // adding extra callbacks just for logging when the JVM is initialized
    // and when it is done performing its tasks
    callbacks.VMInit = VMInit;
    jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                    JVMTI_EVENT_VM_INIT,
                                    NULL);
    callbacks.VMDeath = VMDeath;
    jvmti->SetEventNotificationMode(JVMTI_ENABLE,
                                    JVMTI_EVENT_VM_DEATH,
                                    NULL);

    if ((err = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)))) {
        fprintf(stderr, "sync_jvmti: Failed to set callbacks (%d)\n", err);
        return 1;
    }

    fprintf(stderr, "sync_jvmti: Agent has been loaded.\n");
    return 0;
}

/// Called by the virtual machine once the agent is about to unload.
JNIEXPORT void JNICALL 
Agent_OnUnload(JavaVM *vm) {
    fprintf(stderr, "sync_jvmti: Agent has been unloaded\n");
}
