#include <node_api.h>
#include <stdio.h>
#include <time.h>
#include <uv.h>

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

class WatchdogThread
{
public:
    WatchdogThread(uint64_t timeout, bool terminate)
    : m_timeout(timeout)
    , m_terminate(terminate)
    , m_running(false)
    , m_triggered(false)
    , m_timestamp(uv_hrtime())
    {
        uv_mutex_init(&m_mutex);
        uv_cond_init(&m_condition);
    }

    ~WatchdogThread()
    {
        uv_cond_destroy(&m_condition);
        uv_mutex_destroy(&m_mutex);
    }

    bool start()
    {
        m_timestamp = uv_hrtime();
        m_triggered = false;
        m_running = true;

        if (uv_thread_create(&m_id, threadEntry, (void*)this))
        {
            m_running = false;
            return false;
        }

        return true;
    }

    bool stop()
    {
        uv_mutex_lock(&m_mutex);

        m_timestamp = uv_hrtime();
        m_running = false;

        uv_cond_signal(&m_condition);
        uv_mutex_unlock(&m_mutex);

        uv_thread_join(&m_id);

        return m_triggered;
    }

    void ping()
    {
        m_timestamp = uv_hrtime();
    }

private:

    void threadFunc()
    {
        do {
            uint64_t now = uv_hrtime();
            if ((now - m_timestamp) > m_timeout)
            {
                m_triggered = true;

                if (m_terminate)
                {
                    // TODO: print callstack
                    exit(87);   // using exit code from MS native watchdog
                }
            }


            uv_mutex_lock(&m_mutex);
            uv_cond_timedwait(&m_condition, &m_mutex, (uint64_t)1e9);
            uv_mutex_unlock(&m_mutex);
        } while (m_running);
    }

    static void threadEntry(void* arg)
    {
        WatchdogThread* instance = (WatchdogThread*)arg;
        instance->threadFunc();
    }

    uint64_t m_timeout;
    bool m_terminate;

    volatile bool m_running;
    volatile bool m_triggered;
    volatile uint64_t m_timestamp;

    uv_mutex_t m_mutex;
    uv_cond_t m_condition;

    uv_thread_t m_id;
};

WatchdogThread* s_thread = nullptr;

bool get_start_arguments(napi_env env, napi_callback_info cbinfo, uint64_t& timeout, bool& terminate)
{
    napi_status status;

    do {
        size_t argc = 1;
        napi_value argv[1];

        status = napi_get_cb_info(env, cbinfo, &argc, argv, nullptr, nullptr);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not get callback info");
            break;
        }

        napi_value timeout_value;
        status = napi_get_named_property(env, argv[0], "timeout", &timeout_value);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not get timeout property");
            break;
        }

        status = napi_get_value_int64(env, timeout_value, (int64_t*)&timeout);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not read timeout property as integer");
            break;
        }

        napi_value terminate_value;
        status = napi_get_named_property(env, argv[0], "terminate", &terminate_value);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not get terminate property");
            break;
        }

        status = napi_get_value_bool(env, terminate_value, &terminate);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not read terminate property as boolean");
            break;
        }

        return true;
    } while (false);

    return false;
}

/*!
 *  start - Start watchdog
 *
 *  arguments:
 *   0 - options object
 *      timeout - watchdog timeout in ms
 *      terminate - if watchdog should terminate if triggered
 *
 *  returns:
 *      watchdog id (currently hardcoded to 1)
**/
napi_value start(napi_env env, napi_callback_info cbinfo)
{
    // setup thread

    if (s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog already running");
        return nullptr;
    }

    uint64_t timeout;
    bool terminate;
    if (!get_start_arguments(env, cbinfo, timeout, terminate))
    {
        return nullptr;
    }

    s_thread = new WatchdogThread(timeout * 1000000, terminate);

    if (!s_thread->start())
    {
        delete s_thread;
        napi_throw_error(env, NULL, "Failed to launch watchdog thread");

        return nullptr;
    }

    napi_value output;
    napi_create_int32(env, 1, &output);

    return output;
}

/*!
 *  stop - Stop watchdog
 *
 *  arguments:
 *      0 - watchdog id (currently unused)
 *  returns:
 *      1 if watchdog was triggered, 0 is it wasn't
**/
napi_value stop(napi_env env, napi_callback_info args)
{
    if (!s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog thread not running");
        return nullptr;
    }

    bool triggered = s_thread->stop();

    delete s_thread;
    s_thread = nullptr;

    napi_value output;
    napi_get_boolean(env, triggered, &output);

    return output;
}

/*!
 *  ping - Update ping timestamp
 *
 *  arguments:
 *      0 - watchdog id (currently unused)
 *  returns:
 *      none
**/
napi_value ping(napi_env env, napi_callback_info args)
{
    if (!s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog thread not running");
        return nullptr;
    }

    s_thread->ping();
    return nullptr;
}

napi_value init(napi_env env, napi_value exports)
{
    do {
        napi_status status;
        napi_value fn;

        status = napi_create_function(env, nullptr, 0, start, nullptr, &fn);
        if (status != napi_ok) break;
        status = napi_set_named_property(env, exports, "start", fn);
        if (status != napi_ok) break;

        status = napi_create_function(env, nullptr, 0, stop, nullptr, &fn);
        if (status != napi_ok) break;
        status = napi_set_named_property(env, exports, "stop", fn);
        if (status != napi_ok) break;

        status = napi_create_function(env, nullptr, 0, ping, nullptr, &fn);
        if (status != napi_ok) break;
        status = napi_set_named_property(env, exports, "ping", fn);
        if (status != napi_ok) break;

        return exports;
    } while (false);

    napi_throw_error(env, nullptr, "Failed to initialize");
    return nullptr;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
