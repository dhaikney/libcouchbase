/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2010, 2011 Couchbase, Inc.
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

/**
 * This file contains the implementations of the callback handlers
 * fired when a packet is received on the wire.
 *
 * @author Trond Norbye
 * @todo add more documentation
 */

#include "internal.h"

void setup_lcb_get_resp_t(lcb_get_resp_t *resp,
                          const void *key,
                          lcb_size_t nkey,
                          const void *bytes,
                          lcb_size_t nbytes,
                          lcb_uint32_t flags,
                          lcb_cas_t cas,
                          lcb_datatype_t datatype)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
    resp->v.v0.bytes = bytes;
    resp->v.v0.nbytes = nbytes;
    resp->v.v0.flags = flags;
    resp->v.v0.cas = cas;
    resp->v.v0.datatype = datatype;
}

void setup_lcb_remove_resp_t(lcb_remove_resp_t *resp,
                             const void *key,
                             lcb_size_t nkey)
{
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
}

void setup_lcb_store_resp_t(lcb_store_resp_t *resp,
                            const void *key,
                            lcb_size_t nkey,
                            lcb_cas_t cas)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
    resp->v.v0.cas = cas;
}

void setup_lcb_touch_resp_t(lcb_touch_resp_t *resp,
                            const void *key,
                            lcb_size_t nkey,
                            lcb_cas_t cas)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
    resp->v.v0.cas = cas;
}

void setup_lcb_unlock_resp_t(lcb_unlock_resp_t *resp,
                             const void *key,
                             lcb_size_t nkey)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
}

void setup_lcb_arithmetic_resp_t(lcb_arithmetic_resp_t *resp,
                                 const void *key,
                                 lcb_size_t nkey,
                                 lcb_uint64_t value,
                                 lcb_cas_t cas)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
    resp->v.v0.value = value;
    resp->v.v0.cas = cas;
}

void setup_lcb_observe_resp_t(lcb_observe_resp_t *resp,
                              const void *key,
                              lcb_size_t nkey,
                              lcb_cas_t cas,
                              lcb_observe_t status,
                              int from_master,
                              lcb_time_t ttp,
                              lcb_time_t ttr)
{
    memset(resp, 0, sizeof(*resp));
    resp->version = 0;
    resp->v.v0.key = key;
    resp->v.v0.nkey = nkey;
    resp->v.v0.cas = cas;
    resp->v.v0.status = status;
    resp->v.v0.from_master = from_master;
    resp->v.v0.ttp = ttp;
    resp->v.v0.ttr = ttr;
}

static lcb_error_t map_error(protocol_binary_response_status in)
{
    switch (in) {
    case PROTOCOL_BINARY_RESPONSE_SUCCESS:
        return LCB_SUCCESS;
    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT:
        return LCB_KEY_ENOENT;
    case PROTOCOL_BINARY_RESPONSE_E2BIG:
        return LCB_E2BIG;
    case PROTOCOL_BINARY_RESPONSE_ENOMEM:
        return LCB_ENOMEM;
    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS:
        return LCB_KEY_EEXISTS;
    case PROTOCOL_BINARY_RESPONSE_EINVAL:
        return LCB_EINVAL;
    case PROTOCOL_BINARY_RESPONSE_NOT_STORED:
        return LCB_NOT_STORED;
    case PROTOCOL_BINARY_RESPONSE_DELTA_BADVAL:
        return LCB_DELTA_BADVAL;
    case PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET:
        return LCB_NOT_MY_VBUCKET;
    case PROTOCOL_BINARY_RESPONSE_AUTH_ERROR:
        return LCB_AUTH_ERROR;
    case PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE:
        return LCB_AUTH_CONTINUE;
    case PROTOCOL_BINARY_RESPONSE_ERANGE:
        return LCB_ERANGE;
    case PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND:
        return LCB_UNKNOWN_COMMAND;
    case PROTOCOL_BINARY_RESPONSE_NOT_SUPPORTED:
        return LCB_NOT_SUPPORTED;
    case PROTOCOL_BINARY_RESPONSE_EINTERNAL:
        return LCB_EINTERNAL;
    case PROTOCOL_BINARY_RESPONSE_EBUSY:
        return LCB_EBUSY;
    case PROTOCOL_BINARY_RESPONSE_ETMPFAIL:
        return LCB_ETMPFAIL;
    default:
        return LCB_ERROR;

    }
}


static void dummy_request_handler(lcb_server_t *server,
                                  struct lcb_command_data_st *command_data,
                                  protocol_binary_request_header *req)
{
    (void)server;
    (void)req;
    (void)command_data;
#ifdef DEBUG
    fprintf(stderr, "Received request packet %02x\n", req->request.opcode);
#endif
}

static void dummy_response_handler(lcb_server_t *server,
                                   struct lcb_command_data_st *command_data,
                                   protocol_binary_response_header *res)
{
#ifdef DEBUG
    fprintf(stderr, "Received response packet %02x %04x\n",
            res->response.opcode, ntohs(res->response.status));
#endif
    (void)server;
    (void)res;
    (void)command_data;
}

/**
 * Get a pointer to the key. If the buffer isn't continous we need to
 * allocate a temporary chunk of memory and copy the packet over there.
 * packet will return the pointer to the newly allocated packet or
 * NULL if we didn't have to allocate anything.
 *
 * @param server the server owning the key
 * @param nkey the number of bytes in the key
 * @param packet where to store the result
 * @return pointer to the key
 */
static const char *get_key(lcb_server_t *server, lcb_uint16_t *nkey,
                           char **packet)
{
    protocol_binary_request_header req;
    lcb_size_t nr = ringbuffer_peek(&server->cmd_log,
                                    req.bytes, sizeof(req));
    lcb_size_t packetsize = ntohl(req.request.bodylen) + (lcb_uint32_t)sizeof(req);
    char *keyptr;
    *packet = server->cmd_log.read_head;
    assert(nr == sizeof(req));

    *nkey = ntohs(req.request.keylen);
    keyptr = *packet + sizeof(req) + req.request.extlen;
    *packet = NULL;

    if (!ringbuffer_is_continous(&server->cmd_log,
                                 RINGBUFFER_READ,
                                 packetsize)) {
        *packet = malloc(packetsize);
        if (*packet == NULL) {
            lcb_error_handler(server->instance, LCB_CLIENT_ENOMEM,
                              NULL);
            return NULL;
        }

        nr = ringbuffer_peek(&server->cmd_log, *packet, packetsize);
        if (nr != packetsize) {
            lcb_error_handler(server->instance, LCB_EINTERNAL,
                              NULL);
            free(*packet);
            return NULL;
        }
        keyptr = *packet + sizeof(req) + req.request.extlen;
    }

    return keyptr;
}

int lcb_lookup_server_with_command(lcb_t instance,
                                   lcb_uint8_t opcode,
                                   lcb_uint32_t opaque,
                                   lcb_server_t *exc)
{
    protocol_binary_request_header cmd;
    lcb_server_t *server;
    lcb_size_t nr, ii;

    for (ii = 0; ii < instance->nservers; ++ii) {
        server = instance->servers + ii;
        nr = ringbuffer_peek(&server->cmd_log, cmd.bytes, sizeof(cmd));
        if (nr == sizeof(cmd) &&
                cmd.request.opcode == opcode &&
                cmd.request.opaque == opaque &&
                server != exc) {
            return (int)ii;
        }
    }
    return -1;
}

static void release_key(lcb_server_t *server, char *packet)
{
    /*
     * Packet is a NIL pointer if we didn't allocate a temporary
     * object.
     */
    free(packet);
    (void)server;
}

static void getq_response_handler(lcb_server_t *server,
                                  struct lcb_command_data_st *command_data,
                                  protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    protocol_binary_response_getq *getq = (void *)res;
    lcb_uint16_t status = ntohs(res->response.status);
    lcb_size_t nbytes = ntohl(res->response.bodylen);
    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);

    nbytes -= res->response.extlen;
    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL, NULL);
        return;
    } else if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        const char *bytes = (const char *)res;
        lcb_get_resp_t resp;
        bytes += sizeof(getq->bytes);
        setup_lcb_get_resp_t(&resp, key, nkey, bytes, nbytes,
                             ntohl(getq->message.body.flags),
                             res->response.cas, res->response.datatype);
        root->callbacks.get(root, command_data->cookie, LCB_SUCCESS,
                            &resp);
    } else {
        lcb_get_resp_t resp;
        setup_lcb_get_resp_t(&resp, key, nkey, NULL, 0, 0,
                             0, res->response.datatype);
        root->callbacks.get(root, command_data->cookie,
                            map_error(status), &resp);
    }
    release_key(server, packet);
}

static void get_replica_response_handler(lcb_server_t *server,
                                         struct lcb_command_data_st *command_data,
                                         protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    protocol_binary_response_get *get = (void *)res;
    lcb_uint16_t status = ntohs(res->response.status);
    lcb_uint16_t nkey = ntohs(res->response.keylen);
    const char *key = (const char *)res;

    key += sizeof(get->bytes);
    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL, NULL);
        return;
    } else if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        const char *bytes = key + nkey;
        lcb_size_t nbytes = ntohl(res->response.bodylen) - nkey - res->response.extlen;
        lcb_get_resp_t resp;
        setup_lcb_get_resp_t(&resp, key, nkey, bytes, nbytes,
                             ntohl(get->message.body.flags),
                             res->response.cas,
                             res->response.datatype);
        root->callbacks.get(root, command_data->cookie, LCB_SUCCESS,
                            &resp);
    } else {
        if (status == PROTOCOL_BINARY_RESPONSE_NOT_MY_VBUCKET) {
            /* the config was updated, start from first replica */
            command_data->replica = 0;
        } else {
            command_data->replica++;
        }
        if (command_data->replica < root->nreplicas) {
            /* try next replica */
            protocol_binary_request_get req;
            int idx = vbucket_get_replica(root->vbucket_config,
                                          command_data->vbucket,
                                          command_data->replica);
            if (idx < 0 || idx > (int)root->nservers) {
                lcb_error_handler(root, LCB_NETWORK_ERROR,
                                  "GET_REPLICA: missing server");
                return;
            }
            server = root->servers + idx;
            memset(&req, 0, sizeof(req));
            req.message.header.request.magic = PROTOCOL_BINARY_REQ;
            req.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
            req.message.header.request.opcode = CMD_GET_REPLICA;
            req.message.header.request.keylen = ntohs((lcb_uint16_t)nkey);
            req.message.header.request.vbucket = ntohs(command_data->vbucket);
            req.message.header.request.bodylen = ntohl((lcb_uint32_t)nkey);
            req.message.header.request.opaque = ++root->seqno;
            lcb_server_retry_packet(server, command_data,
                                    req.bytes, sizeof(req.bytes));
            lcb_server_write_packet(server, key, nkey);
            lcb_server_end_packet(server);
            lcb_server_send_packets(server);
        } else {
            /* give up and report the error */
            lcb_get_resp_t resp;
            setup_lcb_get_resp_t(&resp, key, nkey, NULL, 0, 0, 0,
                                 res->response.datatype);
            root->callbacks.get(root, command_data->cookie,
                                map_error(status), &resp);
        }
    }
}

static void delete_response_handler(lcb_server_t *server,
                                    struct lcb_command_data_st *command_data,
                                    protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);

    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL, NULL);
    } else {
        lcb_remove_resp_t resp;
        setup_lcb_remove_resp_t(&resp, key, nkey);
        root->callbacks.remove(root, command_data->cookie,
                               map_error(status), &resp);
        release_key(server, packet);
    }
}

static void observe_response_handler(lcb_server_t *server,
                                     struct lcb_command_data_st *command_data,
                                     protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    lcb_uint32_t ttp;
    lcb_uint32_t ttr;
    VBUCKET_CONFIG_HANDLE config;
    const char *end, *ptr = (const char *)&res->response.cas;

    memcpy(&ttp, ptr, sizeof(ttp));
    ttp = ntohl(ttp);
    memcpy(&ttr, ptr + sizeof(ttp), sizeof(ttr));
    ttr = ntohl(ttr);

    ptr = (const char *)res + sizeof(res->bytes);
    end = ptr + ntohl(res->response.bodylen);
    config = root->vbucket_config;
    while (ptr < end) {
        lcb_cas_t cas;
        lcb_uint8_t obs;
        lcb_uint16_t nkey, vb;
        const char *key;
        lcb_observe_resp_t resp;

        memcpy(&vb, ptr, sizeof(vb));
        vb = ntohs(vb);
        ptr += sizeof(vb);
        memcpy(&nkey, ptr, sizeof(nkey));
        nkey = ntohs(nkey);
        ptr += sizeof(nkey);
        key = (const char *)ptr;
        ptr += nkey;
        obs = *((lcb_uint8_t *)ptr);
        ptr += sizeof(obs);
        memcpy(&cas, ptr, sizeof(cas));
        ptr += sizeof(cas);

        setup_lcb_observe_resp_t(&resp, key, nkey, cas, obs,
                                 server->index == vbucket_get_master(config, vb),
                                 ttp, ttr);

        root->callbacks.observe(root, command_data->cookie, map_error(status),
                                &resp);
    }
    /* run callback with null-null-null to signal the end of transfer */
    if (lcb_lookup_server_with_command(root, CMD_OBSERVE,
                                       res->response.opaque, server) < 0) {

        lcb_observe_resp_t resp;
        memset(&resp, 0, sizeof(resp));
        root->callbacks.observe(root, command_data->cookie, map_error(status),
                                &resp);
    }
}

static void store_response_handler(lcb_server_t *server,
                                   struct lcb_command_data_st *command_data,
                                   protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_storage_t op;

    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);


    lcb_uint16_t status = ntohs(res->response.status);

    switch (res->response.opcode) {
    case PROTOCOL_BINARY_CMD_ADD:
        op = LCB_ADD;
        break;
    case PROTOCOL_BINARY_CMD_REPLACE:
        op = LCB_REPLACE;
        break;
    case PROTOCOL_BINARY_CMD_SET:
        op = LCB_SET;
        break;
    case PROTOCOL_BINARY_CMD_APPEND:
        op = LCB_APPEND;
        break;
    case PROTOCOL_BINARY_CMD_PREPEND:
        op = LCB_PREPEND;
        break;
    default:
        /*
        ** It is impossible to get here (since we're called from our
        ** lookup table... If we _DO_ get here, it must be a development
        ** version where the developer isn't done yet (and should be
        ** forced to think about what to do...)
        */
        lcb_error_handler(root, LCB_EINTERNAL,
                          "Internal error. Received an illegal command opcode");
        abort();
    }

    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL,
                          NULL);
    } else {
        lcb_store_resp_t resp;
        setup_lcb_store_resp_t(&resp, key, nkey, res->response.cas);
        root->callbacks.store(root, command_data->cookie, op,
                              map_error(status), &resp);
        release_key(server, packet);
    }
}

static void arithmetic_response_handler(lcb_server_t *server,
                                        struct lcb_command_data_st *command_data,
                                        protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);
    lcb_arithmetic_resp_t resp;
    lcb_uint64_t value = 0;

    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL,
                          NULL);
        return ;
    }

    if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        memcpy(&value, res + 1, sizeof(value));
        value = ntohll(value);
    }

    setup_lcb_arithmetic_resp_t(&resp, key, nkey, value, res->response.cas);
    root->callbacks.arithmetic(root, command_data->cookie,
                               map_error(status), &resp);
    release_key(server, packet);
}

static void stat_response_handler(lcb_server_t *server,
                                  struct lcb_command_data_st *command_data,
                                  protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    lcb_uint16_t nkey;
    lcb_uint32_t nvalue;
    const char *key, *value;

    if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        nkey = ntohs(res->response.keylen);
        if (nkey == 0) {
            if (lcb_lookup_server_with_command(root, PROTOCOL_BINARY_CMD_STAT,
                                               res->response.opaque, server) < 0) {
                /* notify client that data is ready */
                root->callbacks.stat(root, command_data->cookie, NULL,
                                     LCB_SUCCESS, NULL, 0, NULL, 0);
            }
            return;
        }
        key = (const char *)res + sizeof(res->bytes);
        nvalue = ntohl(res->response.bodylen) - nkey;
        value = key + nkey;
        root->callbacks.stat(root, command_data->cookie, server->authority,
                             map_error(status), key, nkey, value, nvalue);
    } else {
        root->callbacks.stat(root, command_data->cookie, server->authority,
                             map_error(status), NULL, 0, NULL, 0);

        /* run callback with null-null-null to signal the end of transfer */
        if (lcb_lookup_server_with_command(root, PROTOCOL_BINARY_CMD_STAT,
                                           res->response.opaque, server) < 0) {
            root->callbacks.stat(root, command_data->cookie, NULL,
                                 LCB_SUCCESS, NULL, 0, NULL, 0);
        }
    }
}

static void verbosity_response_handler(lcb_server_t *server,
                                       struct lcb_command_data_st *command_data,
                                       protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);

    root->callbacks.verbosity(root, command_data->cookie, server->authority,
                              map_error(status));

    /* run callback with null-null-null to signal the end of transfer */
    if (lcb_lookup_server_with_command(root, PROTOCOL_BINARY_CMD_VERBOSITY,
                                       res->response.opaque, server) < 0) {
        root->callbacks.verbosity(root, command_data->cookie, NULL,
                                  LCB_SUCCESS);
    }
}

static void version_response_handler(lcb_server_t *server,
                                     struct lcb_command_data_st *command_data,
                                     protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    lcb_uint32_t nvstring = ntohl(res->response.bodylen);
    const char *vstring;

    if (nvstring) {
        vstring = (const char *)res + sizeof(res->bytes);
    } else {
        vstring = NULL;
    }

    root->callbacks.version(root, command_data->cookie, server->authority, map_error(status),
                            vstring, nvstring);

    if (lcb_lookup_server_with_command(root, PROTOCOL_BINARY_CMD_VERSION,
                                       res->response.opaque, server) < 0) {
        root->callbacks.version(root, command_data->cookie, NULL, LCB_SUCCESS, NULL, 0);
    }

}

static void sasl_list_mech_response_handler(lcb_server_t *server,
                                            struct lcb_command_data_st *command_data,
                                            protocol_binary_response_header *res)
{
    const char *data;
    const char *chosenmech;
    char *mechlist;
    unsigned int len;
    protocol_binary_request_no_extras req;
    lcb_size_t keylen;
    lcb_size_t bodysize;

    assert(ntohs(res->response.status) == PROTOCOL_BINARY_RESPONSE_SUCCESS);
    bodysize = ntohl(res->response.bodylen);
    mechlist = calloc(bodysize + 1, sizeof(char));
    if (mechlist == NULL) {
        lcb_error_handler(server->instance, LCB_CLIENT_ENOMEM, NULL);
        return;
    }
    memcpy(mechlist, (const char *)(res + 1), bodysize);
    if (sasl_client_start(server->sasl_conn, mechlist,
                          NULL, &data, &len, &chosenmech) != SASL_OK) {
        free(mechlist);
        lcb_error_handler(server->instance, LCB_AUTH_ERROR,
                          "Unable to start sasl client");
        return;
    }
    free(mechlist);

    keylen = strlen(chosenmech);
    bodysize = keylen + len;

    memset(&req, 0, sizeof(req));
    req.message.header.request.magic = PROTOCOL_BINARY_REQ;
    req.message.header.request.opcode = PROTOCOL_BINARY_CMD_SASL_AUTH;
    req.message.header.request.keylen = ntohs((lcb_uint16_t)keylen);
    req.message.header.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.message.header.request.bodylen = ntohl((lcb_uint32_t)(bodysize));

    lcb_server_buffer_start_packet(server, command_data->cookie, &server->output,
                                   &server->output_cookies,
                                   req.bytes, sizeof(req.bytes));
    lcb_server_buffer_write_packet(server, &server->output,
                                   chosenmech, keylen);
    lcb_server_buffer_write_packet(server, &server->output, data, len);
    lcb_server_buffer_end_packet(server, &server->output);

    /* send the data and add a write handler */
    lcb_server_event_handler(0, LCB_WRITE_EVENT, server);

    /* Make it known that this was a success. */
    lcb_error_handler(server->instance, LCB_SUCCESS, NULL);
}

static void sasl_auth_response_handler(lcb_server_t *server,
                                       struct lcb_command_data_st *command_data,
                                       protocol_binary_response_header *res)
{
    lcb_uint16_t ret = ntohs(res->response.status);
    if (ret == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        sasl_dispose(&server->sasl_conn);
        server->sasl_conn = NULL;
        lcb_server_connected(server);
    } else if (ret == PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE) {
        /* I don't know how to step yet ;-) */
        lcb_error_handler(server->instance,
                          LCB_NOT_SUPPORTED,
                          "We don't support sasl authentication that requires \"SASL STEP\" yet");
    } else {
        lcb_error_handler(server->instance, LCB_AUTH_ERROR,
                          "SASL authentication failed");
    }

    /* Make it known that this was a success. */
    lcb_error_handler(server->instance, LCB_SUCCESS, NULL);
    (void)command_data;
}

static void sasl_step_response_handler(lcb_server_t *server,
                                       struct lcb_command_data_st *command_data,
                                       protocol_binary_response_header *res)
{
    (void)server;
    (void)res;
    (void)command_data;

    /* I don't have sasl step support yet ;-) */
    lcb_error_handler(server->instance, LCB_NOT_SUPPORTED,
                      "SASL AUTH CONTINUE not supported yet");

#if 0
    // I should put the server to the notification!
    if (server->instance->vbucket_state_listener != NULL) {
        server->instance->vbucket_state_listener(server);
    }
#endif
}

static void touch_response_handler(lcb_server_t *server,
                                   struct lcb_command_data_st *command_data,
                                   protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);
    lcb_uint16_t status = ntohs(res->response.status);

    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL,
                          NULL);
    } else {
        lcb_touch_resp_t resp;
        setup_lcb_touch_resp_t(&resp, key, nkey, res->response.cas);
        root->callbacks.touch(root, command_data->cookie,
                              map_error(status), &resp);
        release_key(server, packet);
    }
}

static void flush_response_handler(lcb_server_t *server,
                                   struct lcb_command_data_st *command_data,
                                   protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    root->callbacks.flush(root, command_data->cookie, server->authority,
                          map_error(status));
    if (lcb_lookup_server_with_command(root, PROTOCOL_BINARY_CMD_FLUSH,
                                       res->response.opaque, server) < 0) {
        root->callbacks.flush(root, command_data->cookie, NULL,
                              LCB_SUCCESS);
    }
}

static void unlock_response_handler(lcb_server_t *server,
                                    struct lcb_command_data_st *command_data,
                                    protocol_binary_response_header *res)
{
    lcb_t root = server->instance;
    lcb_uint16_t status = ntohs(res->response.status);
    char *packet;
    lcb_uint16_t nkey;
    const char *key = get_key(server, &nkey, &packet);

    if (key == NULL) {
        lcb_error_handler(server->instance, LCB_EINTERNAL,
                          NULL);
    } else {
        lcb_unlock_resp_t resp;
        setup_lcb_unlock_resp_t(&resp, key, nkey);
        root->callbacks.unlock(root, command_data->cookie, map_error(status),
                               &resp);
        release_key(server, packet);
    }
}

static void dummy_error_callback(lcb_t instance,
                                 lcb_error_t error,
                                 const char *errinfo)
{
    (void)instance;
    (void)error;
    (void)errinfo;
}

static void dummy_stat_callback(lcb_t instance,
                                const void *cookie,
                                const char *server_endpoint,
                                lcb_error_t error,
                                const void *key,
                                lcb_size_t nkey,
                                const void *value,
                                lcb_size_t nvalue)
{
    (void)instance;
    (void)error;
    (void)cookie;
    (void)server_endpoint;
    (void)key;
    (void)nkey;
    (void)value;
    (void)nvalue;
}

static void dummy_version_callback(lcb_t instance,
                                   const void *cookie,
                                   const char *server_endpoint,
                                   lcb_error_t error,
                                   const char *vstring,
                                   lcb_size_t nvstring)
{
    (void)instance;
    (void)cookie;
    (void)server_endpoint;
    (void)error;
    (void)vstring;
    (void)nvstring;
}

static void dummy_get_callback(lcb_t instance,
                               const void *cookie,
                               lcb_error_t error,
                               const lcb_get_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_store_callback(lcb_t instance,
                                 const void *cookie,
                                 lcb_storage_t operation,
                                 lcb_error_t error,
                                 const lcb_store_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)operation;
    (void)error;
    (void)resp;
}

static void dummy_arithmetic_callback(lcb_t instance,
                                      const void *cookie,
                                      lcb_error_t error,
                                      const lcb_arithmetic_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_remove_callback(lcb_t instance,
                                  const void *cookie,
                                  lcb_error_t error,
                                  const lcb_remove_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_touch_callback(lcb_t instance,
                                 const void *cookie,
                                 lcb_error_t error,
                                 const lcb_touch_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_flush_callback(lcb_t instance,
                                 const void *cookie,
                                 const char *server_endpoint,
                                 lcb_error_t error)
{
    (void)instance;
    (void)cookie;
    (void)server_endpoint;
    (void)error;
}

static void dummy_view_complete_callback(lcb_http_request_t request,
                                         lcb_t instance,
                                         const void *cookie,
                                         lcb_error_t error,
                                         const lcb_http_resp_t *resp)
{
    (void)request;
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_management_data_callback(lcb_http_request_t request,
                                           lcb_t instance,
                                           const void *cookie,
                                           lcb_error_t error,
                                           const lcb_http_resp_t *resp)
{
    (void)request;
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_management_complete_callback(lcb_http_request_t request,
                                               lcb_t instance,
                                               const void *cookie,
                                               lcb_error_t error,
                                               const lcb_http_resp_t *resp)
{
    (void)request;
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_view_data_callback(lcb_http_request_t request,
                                     lcb_t instance,
                                     const void *cookie,
                                     lcb_error_t error,
                                     const lcb_http_resp_t *resp)
{
    (void)request;
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_unlock_callback(lcb_t instance,
                                  const void *cookie,
                                  lcb_error_t error,
                                  const lcb_unlock_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

static void dummy_configuration_callback(lcb_t instance,
                                         lcb_configuration_t val)
{
    (void)instance;
    (void)val;
}

static void dummy_observe_callback(lcb_t instance,
                                   const void *cookie,
                                   lcb_error_t error,
                                   const lcb_observe_resp_t *resp)
{
    (void)instance;
    (void)cookie;
    (void)error;
    (void)resp;
}

void lcb_initialize_packet_handlers(lcb_t instance)
{
    int ii;
    for (ii = 0; ii < 0x100; ++ii) {
        instance->request_handler[ii] = dummy_request_handler;
        instance->response_handler[ii] = dummy_response_handler;
    }

    instance->callbacks.get = dummy_get_callback;
    instance->callbacks.store = dummy_store_callback;
    instance->callbacks.arithmetic = dummy_arithmetic_callback;
    instance->callbacks.remove = dummy_remove_callback;
    instance->callbacks.touch = dummy_touch_callback;
    instance->callbacks.error = dummy_error_callback;
    instance->callbacks.stat = dummy_stat_callback;
    instance->callbacks.version = dummy_version_callback;
    instance->callbacks.view_complete = dummy_view_complete_callback;
    instance->callbacks.view_data = dummy_view_data_callback;
    instance->callbacks.management_complete = dummy_management_complete_callback;
    instance->callbacks.management_data = dummy_management_data_callback;
    instance->callbacks.flush = dummy_flush_callback;
    instance->callbacks.unlock = dummy_unlock_callback;
    instance->callbacks.configuration = dummy_configuration_callback;
    instance->callbacks.observe = dummy_observe_callback;

    instance->response_handler[PROTOCOL_BINARY_CMD_FLUSH] = flush_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_GETQ] = getq_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_GATQ] = getq_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_GET] = getq_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_GAT] = getq_response_handler;
    instance->response_handler[CMD_GET_LOCKED] = getq_response_handler;
    instance->response_handler[CMD_GET_REPLICA] = get_replica_response_handler;
    instance->response_handler[CMD_UNLOCK_KEY] = unlock_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_ADD] = store_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_DELETE] = delete_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_REPLACE] = store_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_SET] = store_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_APPEND] = store_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_PREPEND] = store_response_handler;

    instance->response_handler[PROTOCOL_BINARY_CMD_INCREMENT] = arithmetic_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_DECREMENT] = arithmetic_response_handler;

    instance->response_handler[PROTOCOL_BINARY_CMD_SASL_LIST_MECHS] = sasl_list_mech_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_SASL_AUTH] = sasl_auth_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_SASL_STEP] = sasl_step_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_TOUCH] = touch_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_STAT] = stat_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_VERSION] = version_response_handler;
    instance->response_handler[PROTOCOL_BINARY_CMD_VERBOSITY] = verbosity_response_handler;
    instance->response_handler[CMD_OBSERVE] = observe_response_handler;
}

LIBCOUCHBASE_API
lcb_get_callback lcb_set_get_callback(lcb_t instance,
                                      lcb_get_callback cb)
{
    lcb_get_callback ret = instance->callbacks.get;
    if (cb != NULL) {
        instance->callbacks.get = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_store_callback lcb_set_store_callback(lcb_t instance,
                                          lcb_store_callback cb)
{
    lcb_store_callback ret = instance->callbacks.store;
    if (cb != NULL) {
        instance->callbacks.store = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_arithmetic_callback lcb_set_arithmetic_callback(lcb_t instance,
                                                    lcb_arithmetic_callback cb)
{
    lcb_arithmetic_callback ret = instance->callbacks.arithmetic;
    if (cb != NULL) {
        instance->callbacks.arithmetic = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_observe_callback lcb_set_observe_callback(lcb_t instance,
                                              lcb_observe_callback cb)
{
    lcb_observe_callback ret = instance->callbacks.observe;
    instance->callbacks.observe = cb;
    return ret;
}

LIBCOUCHBASE_API
lcb_remove_callback lcb_set_remove_callback(lcb_t instance,
                                            lcb_remove_callback cb)
{
    lcb_remove_callback ret = instance->callbacks.remove;
    if (cb != NULL) {
        instance->callbacks.remove = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_touch_callback lcb_set_touch_callback(lcb_t instance,
                                          lcb_touch_callback cb)
{
    lcb_touch_callback ret = instance->callbacks.touch;
    if (cb != NULL) {
        instance->callbacks.touch = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_stat_callback lcb_set_stat_callback(lcb_t instance,
                                        lcb_stat_callback cb)
{
    lcb_stat_callback ret = instance->callbacks.stat;
    if (cb != NULL) {
        instance->callbacks.stat = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_version_callback lcb_set_version_callback(lcb_t instance,
                                              lcb_version_callback cb)
{
    lcb_version_callback ret = instance->callbacks.version;
    if (cb != NULL) {
        instance->callbacks.version = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_error_callback lcb_set_error_callback(lcb_t instance,
                                          lcb_error_callback cb)
{
    lcb_error_callback ret = instance->callbacks.error;
    if (cb != NULL) {
        instance->callbacks.error = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_flush_callback lcb_set_flush_callback(lcb_t instance,
                                          lcb_flush_callback cb)
{
    lcb_flush_callback ret = instance->callbacks.flush;
    if (cb != NULL) {
        instance->callbacks.flush = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_http_complete_callback lcb_set_view_complete_callback(lcb_t instance,
                                                          lcb_http_complete_callback cb)
{
    lcb_http_complete_callback ret = instance->callbacks.view_complete;
    if (cb != NULL) {
        instance->callbacks.view_complete = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_http_data_callback lcb_set_view_data_callback(lcb_t instance,
                                                  lcb_http_data_callback cb)
{
    lcb_http_data_callback ret = instance->callbacks.view_data;
    if (cb != NULL) {
        instance->callbacks.view_data = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_http_complete_callback lcb_set_management_complete_callback(lcb_t instance,
                                                                lcb_http_complete_callback cb)
{
    lcb_http_complete_callback ret = instance->callbacks.management_complete;
    if (cb != NULL) {
        instance->callbacks.management_complete = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_http_data_callback lcb_set_management_data_callback(lcb_t instance,
                                                        lcb_http_data_callback cb)
{
    lcb_http_data_callback ret = instance->callbacks.management_data;
    if (cb != NULL) {
        instance->callbacks.management_data = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_unlock_callback lcb_set_unlock_callback(lcb_t instance,
                                            lcb_unlock_callback cb)
{
    lcb_unlock_callback ret = instance->callbacks.unlock;
    if (cb != NULL) {
        instance->callbacks.unlock = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_configuration_callback lcb_set_configuration_callback(lcb_t instance,
                                                          lcb_configuration_callback cb)
{
    lcb_configuration_callback ret = instance->callbacks.configuration;
    if (cb != NULL) {
        instance->callbacks.configuration = cb;
    }
    return ret;
}

LIBCOUCHBASE_API
lcb_verbosity_callback lcb_set_verbosity_callback(lcb_t instance,
                                                  lcb_verbosity_callback cb)
{
    lcb_verbosity_callback ret = instance->callbacks.verbosity;
    if (cb != NULL) {
        instance->callbacks.verbosity = cb;
    }
    return ret;
}
