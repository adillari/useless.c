#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "concord/discord.h"

void on_message_create(struct discord *client, const struct discord_message *event) {
  if (event->author->bot) return;

  struct discord_create_message params = {
    .content = event->content,
    .message_reference = &(struct discord_message_reference) {
      .message_id = event->id,
      .channel_id = event->channel_id,
      .guild_id = event->guild_id,
    },
  };

  discord_create_message(client, event->channel_id, &params, NULL);
}

int main(int argc, char *argv[]) {
  const char *config_file;
  if (argc > 1)
    config_file = argv[1];
  else
    config_file = "./config.json";

  ccord_global_init();
  struct discord *client = discord_config_init(config_file);
  assert(NULL != client && "Couldn't initialize client");

  discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);

  discord_set_on_message_create(client, &on_message_create);

  discord_run(client);

  discord_cleanup(client);
  ccord_global_cleanup();
}
