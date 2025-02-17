#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <curl/curl.h>
#include "concord/discord.h"

#define RANDOM_FACTS_URL "https://api.viewbits.com/v1/uselessfacts?mode=random"

struct memory {
  char *response;
  size_t size;
};

static size_t write_response(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)clientp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  // out of memory
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;
}

static char* get_start(char *response) {
  const char *key = "\"text\":";
  char *start = strstr(response, key);

  return (start + 8);
}

static int calculate_length(char *start) {
  char *stop = strstr(start, "\"");

  return (stop - start);
}

void on_message_create(struct discord *client, const struct discord_message *event) {
  if (strcmp(event->content, "fact") != 0) return;

  struct memory chunk = {0};
  CURLcode res;
  CURL *curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, RANDOM_FACTS_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    res = curl_easy_perform(curl);

    char *fact_start = get_start(chunk.response);
    assert(fact_start != NULL);
    int fact_length = calculate_length(fact_start);
    char fact[fact_length];
    strncpy(fact, fact_start, fact_length);
    fact[fact_length] = '\0';

    if (res == CURLE_OK) {
      struct discord_create_message params = {
        .content = fact,
        .message_reference = &(struct discord_message_reference) {
          .message_id = event->id,
          .channel_id = event->channel_id,
          .guild_id = event->guild_id,
        },
      };
      discord_create_message(client, event->channel_id, &params, NULL);
    }

    free(chunk.response);
    curl_easy_cleanup(curl);
  }
}

int main(void) {
  ccord_global_init();
  struct discord *client = discord_config_init("./config.json"); // CONFIG_FILE
  assert(client || "Couldn't initialize client");

  discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);

  discord_set_on_message_create(client, on_message_create);

  discord_run(client);

  discord_cleanup(client);
  ccord_global_cleanup();

  return 0;
}
