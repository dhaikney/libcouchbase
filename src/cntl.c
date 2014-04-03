/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2010-2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include "internal.h"
#include "bucketconfig/clconfig.h"
#include <lcbio/iotable.h>
#include <mcserver/negotiate.h>
/**
 * ioctl/fcntl-like interface for libcouchbase configuration properties
 */

typedef lcb_error_t (*ctl_handler)(int, lcb_t, int, void *);


static lcb_uint32_t *get_timeout_field(lcb_t instance, int cmd)
{
    lcb_settings *settings = instance->settings;
    switch (cmd) {

    case LCB_CNTL_OP_TIMEOUT:
        return &settings->operation_timeout;

    case LCB_CNTL_VIEW_TIMEOUT:
        return &settings->views_timeout;

    case LCB_CNTL_DURABILITY_INTERVAL:
        return &settings->durability_interval;

    case LCB_CNTL_DURABILITY_TIMEOUT:
        return &settings->durability_timeout;

    case LCB_CNTL_HTTP_TIMEOUT:
        return &settings->http_timeout;

    case LCB_CNTL_CONFIGURATION_TIMEOUT:
        return &settings->config_timeout;

    case LCB_CNTL_CONFDELAY_THRESH:
        return &settings->weird_things_delay;

    case LCB_CNTL_CONFIG_NODE_TIMEOUT:
        return &settings->config_node_timeout;

    case LCB_CNTL_HTCONFIG_IDLE_TIMEOUT:
        return &settings->bc_http_stream_time;

    default:
        return NULL;
    }
}

static lcb_error_t timeout_common(int mode,
                                  lcb_t instance, int cmd, void *arg)
{
    lcb_uint32_t *ptr;
    lcb_uint32_t *user = arg;

    ptr = get_timeout_field(instance, cmd);
    if (!ptr) {
        return LCB_EINVAL;
    }
    if (mode == LCB_CNTL_GET) {
        *user = *ptr;
    } else {
        *ptr = *user;
    }
    return LCB_SUCCESS;
}

static lcb_error_t bufsize_common(int mode, lcb_t instance, int cmd, void *arg)
{
    lcb_size_t *ptr;
    lcb_size_t *user = arg;

    switch (cmd) {
    case LCB_CNTL_WBUFSIZE:
        ptr = &instance->settings->wbufsize;
        break;

    case LCB_CNTL_RBUFSIZE:
        ptr = &instance->settings->rbufsize;
        break;

    default:
        return LCB_EINVAL;
    }

    if (mode == LCB_CNTL_GET) {
        *user = *ptr;
        return LCB_SUCCESS;

    } else {
        if (!*user) {
            return LCB_EINVAL;
        }
        *ptr = *user;
        return LCB_SUCCESS;
    }
}

static lcb_error_t get_vbconfig(int mode, lcb_t instance, int cmd, void *arg)
{
    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }
    *(VBUCKET_CONFIG_HANDLE *)arg = LCBT_VBCONFIG(instance);

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t get_htype(int mode, lcb_t instance, int cmd, void *arg)
{
    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }

    *(lcb_type_t *)arg = instance->type;

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t get_kvb(int mode, lcb_t instance, int cmd, void *arg)
{
    struct lcb_cntl_vbinfo_st *vbi = arg;

    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }

    if (!LCBT_VBCONFIG(instance)) {
        return LCB_CLIENT_ETMPFAIL;
    }

    if (vbi->version != 0) {
        return LCB_EINVAL;
    }

    vbucket_map(LCBT_VBCONFIG(instance),
                vbi->v.v0.key,
                vbi->v.v0.nkey,
                &vbi->v.v0.vbucket,
                &vbi->v.v0.server_index);

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t get_iops(int mode, lcb_t instance, int cmd, void *arg)
{
    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }

    *(lcb_io_opt_t *)arg = instance->iotable->p;
    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t conninfo(int mode, lcb_t instance, int cmd, void *arg)
{
    lcbio_SOCKET *sock;
    struct lcb_cntl_server_st *si = arg;
    const lcb_host_t *host;

    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }

    if (si->version < 0 || si->version > 1) {
        return LCB_EINVAL;

    }

    if (cmd == LCB_CNTL_MEMDNODE_INFO) {
        lcb_server_t *server;
        int ix = si->v.v0.index;
        if (ix < 0 || ix > (int)LCBT_NSERVERS(instance)) {
            return LCB_EINVAL;
        }

        server = LCBT_GET_SERVER(instance, ix);
        if (!server) {
            return LCB_NETWORK_ERROR;
        }

        sock = server->connctx->sock;

        if (si->version == 1 && sock) {
            mc_pSASLINFO sasl = mc_sasl_get(server->connctx->sock);
            if (sasl) {
                si->v.v1.sasl_mech = mc_sasl_getmech(sasl);
            }
        }


    } else if (cmd == LCB_CNTL_CONFIGNODE_INFO) {
        sock = lcb_confmon_get_rest_connection(instance->confmon);

    } else {
        return LCB_EINVAL;
    }

    if (!sock) {
        return LCB_SUCCESS;
    }

    host = lcbio_get_host(sock);
    si->v.v0.connected = 1;
    si->v.v0.host = host->host;
    si->v.v0.port = host->port;

    switch (instance->iotable->model) {
    case LCB_IOMODEL_EVENT:
        si->v.v0.sock.sockfd = sock->u.fd;
        break;

    case LCB_IOMODEL_COMPLETION:
        si->v.v0.sock.sockptr = sock->u.sd;
        break;

    default:
        return LCB_NOT_SUPPORTED;
    }

    return LCB_SUCCESS;
}

static lcb_error_t syncmode(int mode, lcb_t instance, int cmd, void *arg)
{
    (void)mode; (void)instance; (void)cmd; (void)arg;
    return LCB_NOT_SUPPORTED;
}

static lcb_error_t ippolicy(int mode, lcb_t instance, int cmd, void *arg)
{
    lcb_ipv6_t *user = arg;

    if (mode == LCB_CNTL_SET) {
        instance->settings->ipv6 = *user;
    } else {
        *user = instance->settings->ipv6;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t confthresh(int mode, lcb_t instance, int cmd, void *arg)
{
    lcb_size_t *user = arg;

    if (mode == LCB_CNTL_SET) {
        instance->settings->weird_things_threshold = *user;
    } else {
        *user = instance->settings->weird_things_threshold;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t bummer_mode_handler(int mode, lcb_t instance, int cmd, void *arg)
{
    int *skip = arg;

    if (mode == LCB_CNTL_SET) {
        instance->settings->bummer = *skip;
    } else {
        *skip = instance->settings->bummer;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t randomize_bootstrap_hosts_handler(int mode,
                                                     lcb_t instance,
                                                     int cmd,
                                                     void *arg)
{
    int *randomize = arg;

    if (cmd != LCB_CNTL_RANDOMIZE_BOOTSTRAP_HOSTS) {
        return LCB_EINTERNAL;
    }

    if (mode == LCB_CNTL_SET) {
        instance->settings->randomize_bootstrap_nodes = *randomize;
    } else {
        *randomize = instance->settings->randomize_bootstrap_nodes;
    }

    return LCB_SUCCESS;
}

static lcb_error_t config_cache_loaded_handler(int mode,
                                               lcb_t instance,
                                               int cmd,
                                               void *arg)
{
    if (mode != LCB_CNTL_GET) {
        return LCB_NOT_SUPPORTED;
    }

    if (cmd != LCB_CNTL_CONFIG_CACHE_LOADED) {
        return LCB_EINTERNAL;
    }

    *(int *)arg = instance->cur_configinfo &&
            instance->cur_configinfo->origin == LCB_CLCONFIG_FILE;

    return LCB_SUCCESS;
}

static lcb_error_t force_sasl_mech_handler(int mode,
                                           lcb_t instance,
                                           int cmd,
                                           void *arg)
{
    union {
        char *set;
        char **get;
    } u_arg;

    u_arg.set = arg;

    if (mode == LCB_CNTL_SET) {
        free(instance->settings->sasl_mech_force);
        if (u_arg.set) {
            instance->settings->sasl_mech_force = strdup(u_arg.set);
        }
    } else {
        *u_arg.get = instance->settings->sasl_mech_force;
    }

    (void)cmd;

    return LCB_SUCCESS;
}

static lcb_error_t max_redirects(int mode, lcb_t instance, int cmd, void *arg)
{
    int *val = arg;

    if (*val < -1) {
        return LCB_EINVAL;
    }
    if (mode == LCB_CNTL_SET) {
        instance->settings->max_redir = *val;
    } else {
        *val = instance->settings->max_redir;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t logprocs_handler(int mode, lcb_t instance, int cmd, void *arg)
{
    if (mode == LCB_CNTL_SET) {
        instance->settings->logger = (lcb_logprocs *)arg;
    } else {
        *(lcb_logprocs**)arg = instance->settings->logger;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t config_transport(int mode, lcb_t instance, int cmd, void *arg)
{
    lcb_config_transport_t *val = arg;

    if (mode == LCB_CNTL_SET) {
        return LCB_EINVAL;
    }

    if (!instance->cur_configinfo) {
        return LCB_CLIENT_ETMPFAIL;
    }

    switch (instance->cur_configinfo->origin) {
        case LCB_CLCONFIG_HTTP:
            *val = LCB_CONFIG_TRANSPORT_HTTP;
            break;

        case LCB_CLCONFIG_CCCP:
            *val = LCB_CONFIG_TRANSPORT_CCCP;
            break;

        default:
            return LCB_CLIENT_ETMPFAIL;
    }

    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t config_nodes(int mode, lcb_t instance, int cmd, void *arg)
{
    const char *node_strs = arg;
    clconfig_provider *target;
    hostlist_t nodes_obj;
    lcb_error_t err;

    if (mode != LCB_CNTL_SET) {
        return LCB_EINVAL;
    }

    nodes_obj = hostlist_create();
    if (!nodes_obj) {
        return LCB_CLIENT_ENOMEM;
    }

    err = hostlist_add_stringz(nodes_obj, node_strs,
                               cmd == LCB_CNTL_CONFIG_HTTP_NODES
                               ? LCB_CONFIG_HTTP_PORT : LCB_CONFIG_MCD_PORT);
    if (err != LCB_SUCCESS) {
        hostlist_destroy(nodes_obj);
        return err;
    }

    if (cmd == LCB_CNTL_CONFIG_HTTP_NODES) {
        target = lcb_confmon_get_provider(instance->confmon, LCB_CLCONFIG_HTTP);
        lcb_clconfig_http_set_nodes(target, nodes_obj);
    } else {
        target = lcb_confmon_get_provider(instance->confmon, LCB_CLCONFIG_CCCP);
        lcb_clconfig_cccp_set_nodes(target, nodes_obj);
    }

    hostlist_destroy(nodes_obj);
    return LCB_SUCCESS;
}

static lcb_error_t get_changeset(int mode, lcb_t instance, int cmd, void *arg)
{
    *(char **)arg = LCB_VERSION_CHANGESET;
    (void)instance;
    (void)mode;
    (void)cmd;
    return LCB_SUCCESS;
}

static lcb_error_t init_providers(int mode, lcb_t instance, int cmd, void *arg)
{
    struct lcb_create_st2 *opts = arg;
    if (mode != LCB_CNTL_SET) {
        return LCB_EINVAL;
    }
    (void)cmd;
    return lcb_init_providers(instance, opts);
}

static ctl_handler handlers[] = {
    timeout_common, /* LCB_CNTL_OP_TIMEOUT */
    timeout_common, /* LCB_CNTL_VIEW_TIMEOUT */
    bufsize_common, /* LCB_CNTL_RBUFSIZE */
    bufsize_common, /* LCB_CNTL_WBUFSIZE */
    get_htype,      /* LCB_CNTL_HANDLETYPE */
    get_vbconfig,   /* LCB_CNTL_VBCONFIG */
    get_iops,       /* LCB_CNTL_IOPS */
    get_kvb,        /* LCB_CNTL_VBMAP */
    conninfo,       /* LCB_CNTL_MEMDNODE_INFO */
    conninfo,       /* LCB_CNTL_CONFIGNODE_INFO */
    syncmode,       /* LCB_CNTL_SYNCMODE */
    ippolicy,       /* LCB_CNTL_IP6POLICY */
    confthresh      /* LCB_CNTL_CONFERRTHRESH */,
    timeout_common, /* LCB_CNTL_DURABILITY_INTERVAL */
    timeout_common, /* LCB_CNTL_DURABILITY_TIMEOUT */
    timeout_common, /* LCB_CNTL_HTTP_TIMEOUT */
    lcb_iops_cntl_handler, /* LCB_CNTL_IOPS_DEFAULT_TYPES */
    lcb_iops_cntl_handler, /* LCB_CNTL_IOPS_DLOPEN_DEBUG */
    timeout_common,  /* LCB_CNTL_CONFIGURATION_TIMEOUT */
    bummer_mode_handler,   /* LCB_CNTL_SKIP_CONFIGURATION_ERRORS_ON_CONNECT */
    randomize_bootstrap_hosts_handler /* LCB_CNTL_RANDOMIZE_BOOTSTRAP_HOSTS */,
    config_cache_loaded_handler /* LCB_CNTL_CONFIG_CACHE_LOADED */,
    force_sasl_mech_handler, /* LCB_CNTL_FORCE_SASL_MECH */
    max_redirects, /* LCB_CNTL_MAX_REDIRECTS */
    logprocs_handler /* LCB_CNTL_LOGGER */,
    timeout_common, /* LCB_CNTL_CONFDELAY_THRESH */
    config_transport, /* LCB_CNTL_CONFIG_TRANSPORT */
    timeout_common, /* LCB_CNTL_CONFIG_NODE_TIMEOUT */
    timeout_common, /* LCB_CNTL_HTCONFIG_IDLE_TIMEOUT */
    config_nodes, /* LCB_CNTL_CONFIG_HTTP_NODES */
    config_nodes, /* LCB_CNTL_CONFIG_CCCP_NODES */
    get_changeset, /* LCB_CNTL_CHANGESET */
    init_providers /* LCB_CNTL_CONFIG_ALL_NODES */
};


LIBCOUCHBASE_API
lcb_error_t lcb_cntl(lcb_t instance, int mode, int cmd, void *arg)
{
    ctl_handler handler;
    if (cmd > LCB_CNTL__MAX) {
        return LCB_NOT_SUPPORTED;
    }

    handler = handlers[cmd];

    if (!handler) {
        return LCB_NOT_SUPPORTED;
    }

    return handler(mode, instance, cmd, arg);
}

LIBCOUCHBASE_API
lcb_error_t lcb_cntl_setu32(lcb_t instance, int cmd, lcb_uint32_t arg)
{
    return lcb_cntl(instance, LCB_CNTL_SET, cmd, &arg);
}

LIBCOUCHBASE_API
lcb_uint32_t lcb_cntl_getu32(lcb_t instance, int cmd)
{
    lcb_uint32_t ret = 0;
    lcb_cntl(instance, LCB_CNTL_GET, cmd, &ret);
    return ret;
}


LIBCOUCHBASE_API
void lcb_behavior_set_ipv6(lcb_t instance, lcb_ipv6_t mode)
{
    lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_IP6POLICY, &mode);
}

LIBCOUCHBASE_API
lcb_ipv6_t lcb_behavior_get_ipv6(lcb_t instance)
{
    lcb_ipv6_t ret;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_IP6POLICY, &ret);
    return ret;
}

LIBCOUCHBASE_API
void lcb_behavior_set_config_errors_threshold(lcb_t instance, lcb_size_t num_events)
{
    lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_CONFERRTHRESH, &num_events);
}

LIBCOUCHBASE_API
lcb_size_t lcb_behavior_get_config_errors_threshold(lcb_t instance)
{
    lcb_size_t ret;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_CONFERRTHRESH, &ret);
    return ret;
}

/**
 * The following is from timeout.c
 */
LIBCOUCHBASE_API
void lcb_set_timeout(lcb_t instance, lcb_uint32_t usec)
{
    lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_OP_TIMEOUT, &usec);
}

LIBCOUCHBASE_API
lcb_uint32_t lcb_get_timeout(lcb_t instance)
{
    lcb_uint32_t ret;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_OP_TIMEOUT, &ret);
    return ret;
}

LIBCOUCHBASE_API
void lcb_set_view_timeout(lcb_t instance, lcb_uint32_t usec)
{
    lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_VIEW_TIMEOUT, &usec);
}

LIBCOUCHBASE_API
lcb_uint32_t lcb_get_view_timeout(lcb_t instance)
{
    lcb_uint32_t ret;
    lcb_cntl(instance, LCB_CNTL_GET, LCB_CNTL_VIEW_TIMEOUT, &ret);
    return ret;
}
