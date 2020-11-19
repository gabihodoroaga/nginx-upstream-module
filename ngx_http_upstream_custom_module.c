#include "ngx_conf_file.h"
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t
ngx_http_upstream_init_custom_peer(ngx_http_request_t *r,
                                 ngx_http_upstream_srv_conf_t *us);

static ngx_int_t 
ngx_http_upstream_get_custom_peer(ngx_peer_connection_t *pc,
                                void *data);

static void 
ngx_http_upstream_free_custom_peer(ngx_peer_connection_t *pc,
                                 void *data, ngx_uint_t state);

static void *ngx_http_upstream_custom_create_conf(ngx_conf_t *cf);

static char *ngx_http_upstream_custom(ngx_conf_t *cf, ngx_command_t *cmd,
                                    void *conf);

typedef struct {
  ngx_uint_t max;
  ngx_http_upstream_init_pt original_init_upstream;
  ngx_http_upstream_init_peer_pt original_init_peer;
} ngx_http_upstream_custom_srv_conf_t;

typedef struct {
  ngx_http_upstream_custom_srv_conf_t *conf;
  ngx_http_upstream_t *upstream;
  void *data;
  ngx_connection_t *client_connection;
  ngx_event_get_peer_pt original_get_peer;
  ngx_event_free_peer_pt original_free_peer;
} ngx_http_upstream_custom_peer_data_t;


/* The module directives */
static ngx_command_t ngx_http_upstream_custom_commands[] = {

    {ngx_string("custom"), NGX_HTTP_UPS_CONF | NGX_CONF_NOARGS | NGX_CONF_TAKE1,
     ngx_http_upstream_custom, NGX_HTTP_SRV_CONF_OFFSET, 0, NULL},

    ngx_null_command /* command termination */
};

/* The module context */
static ngx_http_module_t ngx_http_upstream_custom_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    ngx_http_upstream_custom_create_conf, /* create server configuration */
    NULL,                               /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

/* The module definition */
ngx_module_t ngx_http_upstream_custom_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_custom_ctx,     /* module context */
    ngx_http_upstream_custom_commands, /* module directives */
    NGX_HTTP_MODULE,                   /* module type */
    NULL,                              /* init master */
    NULL,                              /* init module */
    NULL,                              /* init process */
    NULL,                              /* init thread */
    NULL,                              /* exit thread */
    NULL,                              /* exit process */
    NULL,                              /* exit master */
    NGX_MODULE_V1_PADDING};


static ngx_int_t ngx_http_upstream_init_custom(ngx_conf_t *cf,
                                             ngx_http_upstream_srv_conf_t *us) {
  ngx_http_upstream_custom_srv_conf_t *hccf;

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0, "custom init upstream");

  hccf = ngx_http_conf_upstream_srv_conf(us, ngx_http_upstream_custom_module);

  ngx_conf_init_uint_value(hccf->max, 100);

  if (hccf->original_init_upstream(cf, us) != NGX_OK) {
    return NGX_ERROR;
  }

  hccf->original_init_peer = us->peer.init;
  us->peer.init = ngx_http_upstream_init_custom_peer;

  return NGX_OK;
}

static ngx_int_t
ngx_http_upstream_init_custom_peer(ngx_http_request_t *r,
                                 ngx_http_upstream_srv_conf_t *us) {
  
  ngx_http_upstream_custom_srv_conf_t *hccf;
  ngx_http_upstream_custom_peer_data_t *hcpd;
  
  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "custom init peer");

  hcpd = ngx_palloc(r->pool, sizeof(ngx_http_upstream_custom_peer_data_t));
  if (hcpd == NULL) {
    return NGX_ERROR;
  }

  hccf = ngx_http_conf_upstream_srv_conf(us, ngx_http_upstream_custom_module);

  if (hccf->original_init_peer(r, us) != NGX_OK) {
    return NGX_ERROR;
  }

  hcpd->conf = hccf;
  hcpd->upstream = r->upstream;
  hcpd->data = r->upstream->peer.data;
  hcpd->client_connection = r->connection;

  hcpd->original_get_peer = r->upstream->peer.get;
  hcpd->original_free_peer = r->upstream->peer.free;

  r->upstream->peer.data = hcpd;
  r->upstream->peer.get = ngx_http_upstream_get_custom_peer;
  r->upstream->peer.free = ngx_http_upstream_free_custom_peer;

  return NGX_OK;
}

static ngx_int_t ngx_http_upstream_get_custom_peer(ngx_peer_connection_t *pc,
                                                 void *data) {
  ngx_http_upstream_custom_peer_data_t *hcdp = data;
  ngx_int_t rc;

  ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                 "custom get peer, try: %ui, conn: %p", pc->tries,
                 hcdp->client_connection);

  rc = hcdp->original_get_peer(pc, hcdp->data);

  if (rc != NGX_OK) {
    return rc;
  }

  /* in this section you can set the upstream server connection */

  return NGX_OK;
}

static void ngx_http_upstream_free_custom_peer(ngx_peer_connection_t *pc,
                                             void *data, ngx_uint_t state) {
  ngx_http_upstream_custom_peer_data_t *hcdp = data;

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, pc->log, 0, "custom free peer");

  hcdp->original_free_peer(pc, hcdp->data, state);
}

static void *ngx_http_upstream_custom_create_conf(ngx_conf_t *cf) {
  ngx_http_upstream_custom_srv_conf_t *conf;
  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_custom_srv_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->max = NGX_CONF_UNSET_UINT;

  return conf;
}

// The main entry point of the module
static char *ngx_http_upstream_custom(ngx_conf_t *cf, ngx_command_t *cmd,
                                    void *conf) {
  ngx_http_upstream_srv_conf_t *uscf;
  ngx_http_upstream_custom_srv_conf_t *hccf = conf;

  ngx_int_t n;
  ngx_str_t *value;

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0, "custom init module");

  /* read options */
  if (cf->args->nelts == 2) {
    value = cf->args->elts;
    n = ngx_atoi(value[1].data, value[1].len);
    if (n == NGX_ERROR || n == 0) {
      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                         "invalid value \"%V\" in \"%V\" directive", &value[1],
                         &cmd->name);
      return NGX_CONF_ERROR;
    }
    hccf->max = n;
  }

  uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

  hccf->original_init_upstream = uscf->peer.init_upstream
                                     ? uscf->peer.init_upstream
                                     : ngx_http_upstream_init_round_robin;

  uscf->peer.init_upstream = ngx_http_upstream_init_custom;

  return NGX_CONF_OK;
}

