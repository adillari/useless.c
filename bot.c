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
  if (strstr(response, "Rate limit exceeded") != NULL) return NULL;

  const char *key = "\"text\":";
  char *start = strstr(response, key);

  return (start + 8);
}

static int calculate_length(char *start) {
  if (start == NULL) return -1;

  char *stop = strstr(start, "\",\"");

  return (stop - start);
}

void on_message_create(struct discord *client, const struct discord_message *event) {
  if (strcmp(event->content, "fact") != 0) return; // bot only responds to "fact"

  struct memory chunk = {0}; // init memory to store api response
  struct discord_message_reference message_reference = { // we will be responding to the "fact" message
    .message_id = event->id,
    .channel_id = event->channel_id,
    .guild_id = event->guild_id,
  };

  char *content = "CURL FAILED TO INIT";
  CURL *curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, RANDOM_FACTS_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    CURLcode res = curl_easy_perform(curl);

    content = "API error";
    if (res == CURLE_OK) {
      content = get_start(chunk.response); // returns NULL if couldn't find start
      int fact_length = calculate_length(content); // returns -1 if content is NULL
      if (fact_length == -1) {
        content = "Too fast! Try again in 30 seconds.";
      } else {
        content[fact_length] = '\0';
      }
    }
  }

  struct discord_create_message params = { .content = content, .message_reference = &message_reference };
  discord_create_message(client, event->channel_id, &params, NULL);

  free(chunk.response);
  curl_easy_cleanup(curl);
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
