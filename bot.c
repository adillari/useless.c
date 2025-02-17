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

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
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

void on_message_create(struct discord *client, const struct discord_message *event) {
  if (strcmp(event->content, "fact") != 0) return;

  struct memory chunk = {0};
  CURLcode res;
  CURL *curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, RANDOM_FACTS_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      struct discord_create_message params = {
        .content = chunk.response,
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
}
