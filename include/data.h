#include <stddef.h>
#include <yyjson.h>

typedef struct {
  double lastPrice;
  double changePercent;
  int volume;
  long long marketCap;
} BrapiQuote;

typedef struct {
  char *symbol;
  char *name;
  char *assetType;
  char *exchange;
  char *currency;
  char *sector;
  bool isActive;
  BrapiQuote quote;
} BrapiResults;

typedef struct {
  BrapiResults *results;
  size_t results_count;
} BrapiTrackerJsonResponse;

typedef struct {
  char *memory;
  size_t size;
} MemoryBuffer;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp);
void free_brapi_response(BrapiTrackerJsonResponse *response);

BrapiTrackerJsonResponse *fetch_brapi_dataset();
BrapiTrackerJsonResponse *fetch_brapi_specific(char *ticker);
