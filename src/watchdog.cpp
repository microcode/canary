#include <node_api.h>
#include <stdio.h>
#include <time.h>
#include <uv.h>

#if !defined(_MSC_VER)
#include <unistd.h>
#endif

#include <stdlib.h>
#include <memory.h>
#include <inttypes.h>

#define EXIT_CODE (87)

class WatchdogThread
{
public:
    WatchdogThread(uint64_t timeout, bool terminate, bool print)
    : m_timeout(timeout)
    , m_terminate(terminate)
    , m_print(print)
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
                if (!m_triggered) {
                    m_triggered = true;

                    if (m_terminate)
                    {
                        if (m_print)
                        {
                            fprintf(stderr, "FATAL: canary - watchdog timeout detected (no ping after %" PRIu64 "ms), exiting application.\n", m_timeout / 1000000);
                        }
                        exit(EXIT_CODE);
                    }
                    else if (m_print)
                    {
                        fprintf(stderr, "canary - watchdog timeout detected (no ping after %" PRIu64 "ms)\n", m_timeout / 1000000);
                    }

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
    bool m_print;

    volatile bool m_running;
    volatile bool m_triggered;
    volatile uint64_t m_timestamp;

    uv_mutex_t m_mutex;
    uv_cond_t m_condition;

    uv_thread_t m_id;
};

WatchdogThread* s_thread = nullptr;

bool get_start_arguments(napi_env env, napi_callback_info info, uint64_t& timeout, bool& terminate, bool& print)
{
    napi_status status;

    do {
        size_t argc = 1;
        napi_value argv[1];

        status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
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

        napi_value print_value;
        status = napi_get_named_property(env, argv[0], "print", &print_value);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not get print property");
            break;
        }

        status = napi_get_value_bool(env, print_value, &print);
        if (status != napi_ok)
        {
            napi_throw_error(env, nullptr, "Could not read print property as boolean");
            break;
        }

        return true;
    } while (false);

    return false;
}

/*!
 *
 *  start - Start watchdog
 *
 *  arguments:
 *   0 - options object
 *      timeout - watchdog timeout in ms
 *      terminate - terminate when triggered
 *      print - print when triggered
 *
**/
napi_value start(napi_env env, napi_callback_info cbinfo)
{
    if (s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog already running");
        return nullptr;
    }

    uint64_t timeout;
    bool terminate, print;
    if (!get_start_arguments(env, cbinfo, timeout, terminate, print))
    {
        return nullptr;
    }

    s_thread = new WatchdogThread(timeout * 1000000, terminate, print);

    if (!s_thread->start())
    {
        delete s_thread;
        s_thread = nullptr;

        napi_throw_error(env, NULL, "Failed to launch watchdog thread");
        return nullptr;
    }

    return nullptr;
}

/*!
 *
 *  stop - Stop watchdog
 *
 *  returns true if watchdog was triggered
 *
**/
napi_value stop(napi_env env, napi_callback_info args)
{
    if (!s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog is not running");
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
 *
 *  ping - Update ping timestamp
 *
**/
napi_value ping(napi_env env, napi_callback_info args)
{
    if (!s_thread)
    {
        napi_throw_error(env, nullptr, "Watchdog is not running");
        return nullptr;
    }

    s_thread->ping();
    return nullptr;
}

#define REGISTER_FUNCTION(NAME)\
    {\
        napi_status status;\
        napi_value fn;\
        status = napi_create_function(env, nullptr, 0, NAME, nullptr, &fn);\
        if (status != napi_ok) break;\
        status = napi_set_named_property(env, exports, #NAME, fn);\
        if (status != napi_ok) break;\
    }

napi_value init(napi_env env, napi_value exports)
{
    do {
        REGISTER_FUNCTION(start);
        REGISTER_FUNCTION(stop);
        REGISTER_FUNCTION(ping);

        return exports;
    } while (false);

    napi_throw_error(env, nullptr, "Failed to initialize");
    return nullptr;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
