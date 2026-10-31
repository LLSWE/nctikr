
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>
#include <data.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  MemoryBuffer *mem = (MemoryBuffer *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    printf("failed to realloc curl mem\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void free_brapi_response(BrapiTrackerJsonResponse *response) {
  if (!response)
    return;
  if (response->results) {
    for (size_t i = 0; i < response->results_count; i++) {
      free(response->results[i].symbol);
      free(response->results[i].name);
      free(response->results[i].assetType);
      free(response->results[i].exchange);
      free(response->results[i].currency);
      free(response->results[i].sector);
    }
    free(response->results);
  }
  free(response);
}

BrapiTrackerJsonResponse *fetch_brapi_dataset() {
  CURL *curl;
  CURLcode res;

  MemoryBuffer chunk = {.memory = malloc(1), .size = 0};
  if (!chunk.memory)
    return NULL;

  curl = curl_easy_init();
  if (!curl) {
    free(chunk.memory);
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL, "https://brapi.dev/api/v2/tickers");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "Erro no cURL: %s\n", curl_easy_strerror(res));
    free(chunk.memory);
    return NULL;
  }

  yyjson_doc *doc = yyjson_read(chunk.memory, chunk.size, 0);
  free(chunk.memory);

  if (!doc) {
    fprintf(stderr, "Erro ao parsear a string JSON com yyjson.\n");
    return NULL;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  yyjson_val *results_array = yyjson_obj_get(root, "results");
  size_t arr_size = yyjson_arr_size(results_array);

  if (arr_size == 0) {
    yyjson_doc_free(doc);
    return NULL;
  }

  BrapiTrackerJsonResponse *response = malloc(sizeof(BrapiTrackerJsonResponse));
  if (!response) {
    yyjson_doc_free(doc);
    return NULL;
  }
  response->results_count = arr_size;
  response->results = calloc(arr_size, sizeof(BrapiResults));

  size_t idx;
  yyjson_val *val;

  yyjson_arr_foreach(results_array, idx, arr_size, val) {
    BrapiResults *res_item = &response->results[idx];

    const char *sym = yyjson_get_str(yyjson_obj_get(val, "symbol"));
    res_item->symbol = sym ? strdup(sym) : NULL;

    const char *nm = yyjson_get_str(yyjson_obj_get(val, "name"));
    res_item->name = nm ? strdup(nm) : NULL;

    const char *at = yyjson_get_str(yyjson_obj_get(val, "assetType"));
    res_item->assetType = at ? strdup(at) : NULL;

    const char *ex = yyjson_get_str(yyjson_obj_get(val, "exchange"));
    res_item->exchange = ex ? strdup(ex) : NULL;

    const char *cur = yyjson_get_str(yyjson_obj_get(val, "currency"));
    res_item->currency = cur ? strdup(cur) : NULL;

    const char *sec = yyjson_get_str(yyjson_obj_get(val, "sector"));
    res_item->sector = sec ? strdup(sec) : NULL;

    res_item->isActive = yyjson_get_bool(yyjson_obj_get(val, "isActive"));

    yyjson_val *quote_obj = yyjson_obj_get(val, "quote");
    if (quote_obj) {
      res_item->quote.lastPrice =
          yyjson_get_num(yyjson_obj_get(quote_obj, "lastPrice"));
      res_item->quote.changePercent =
          yyjson_get_num(yyjson_obj_get(quote_obj, "changePercent"));
      res_item->quote.volume =
          yyjson_get_int(yyjson_obj_get(quote_obj, "volume"));
      res_item->quote.marketCap =
          yyjson_get_sint(yyjson_obj_get(quote_obj, "marketCap"));
    }
  }

  yyjson_doc_free(doc);
  return response;
}

BrapiTrackerJsonResponse *fetch_brapi_specific(char *ticker) {
  CURL *curl;
  CURLcode res;

  MemoryBuffer chunk = {.memory = malloc(1), .size = 0};
  if (!chunk.memory)
    return NULL;

  curl = curl_easy_init();
  if (!curl) {
    free(chunk.memory);
    return NULL;
  }

  char url[512];
  snprintf(url, sizeof(url), "https://brapi.dev/api/v2/tickers?search=%s",
           ticker);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "Erro no cURL: %s\n", curl_easy_strerror(res));
    free(chunk.memory);
    return NULL;
  }

  yyjson_doc *doc = yyjson_read(chunk.memory, chunk.size, 0);
  free(chunk.memory);

  if (!doc) {
    fprintf(stderr, "Erro ao parsear a string JSON com yyjson.\n");
    return NULL;
  }

  yyjson_val *root = yyjson_doc_get_root(doc);
  yyjson_val *results_array = yyjson_obj_get(root, "results");
  size_t arr_size = yyjson_arr_size(results_array);

  if (arr_size == 0) {
    yyjson_doc_free(doc);
    return NULL;
  }

  BrapiTrackerJsonResponse *response = malloc(sizeof(BrapiTrackerJsonResponse));
  if (!response) {
    yyjson_doc_free(doc);
    return NULL;
  }
  response->results_count = arr_size;
  response->results = calloc(arr_size, sizeof(BrapiResults));

  size_t idx;
  yyjson_val *val;

  yyjson_arr_foreach(results_array, idx, arr_size, val) {
    BrapiResults *res_item = &response->results[idx];

    const char *sym = yyjson_get_str(yyjson_obj_get(val, "symbol"));
    res_item->symbol = sym ? strdup(sym) : NULL;

    const char *nm = yyjson_get_str(yyjson_obj_get(val, "name"));
    res_item->name = nm ? strdup(nm) : NULL;

    const char *at = yyjson_get_str(yyjson_obj_get(val, "assetType"));
    res_item->assetType = at ? strdup(at) : NULL;

    const char *ex = yyjson_get_str(yyjson_obj_get(val, "exchange"));
    res_item->exchange = ex ? strdup(ex) : NULL;

    const char *cur = yyjson_get_str(yyjson_obj_get(val, "currency"));
    res_item->currency = cur ? strdup(cur) : NULL;

    const char *sec = yyjson_get_str(yyjson_obj_get(val, "sector"));
    res_item->sector = sec ? strdup(sec) : NULL;

    res_item->isActive = yyjson_get_bool(yyjson_obj_get(val, "isActive"));

    yyjson_val *quote_obj = yyjson_obj_get(val, "quote");
    if (quote_obj) {
      res_item->quote.lastPrice =
          yyjson_get_num(yyjson_obj_get(quote_obj, "lastPrice"));
      res_item->quote.changePercent =
          yyjson_get_num(yyjson_obj_get(quote_obj, "changePercent"));
      res_item->quote.volume =
          yyjson_get_int(yyjson_obj_get(quote_obj, "volume"));
      res_item->quote.marketCap =
          yyjson_get_sint(yyjson_obj_get(quote_obj, "marketCap"));
    }
  }

  yyjson_doc_free(doc);
  return response;
}
