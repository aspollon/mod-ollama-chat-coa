# Roleplay sheets (2026-11-11)

A sheet like Total RP's for every character: players write theirs in the client add-on AFKRoleplay, the language
model writes the bots'. Bots read both when they talk (`Rp_PromptSection`): a bot keeps to its own sheet and knows
what the player shows of themselves (name, title, age, home, looks, manner, history, right now).

## Where sheets come from

- **Players**: the add-on's editor (`/rp edit`); sent on save and once per login, kept in `mod_ollama_chat_rp`.
- **Bots**: written once from the bot's race, class, faction, sex and temperament, then kept. Asked for when
  someone opens the sheet ("being written" until it arrives, then it is pushed to the window), and written ahead
  while the model has nothing else to do: first the bots in a real player's group, then bots near one
  (`OllamaChat.RP.Pregenerate*`, at most `PregeneratePerHour`). One at a time, never past queued chat.

## Wire format

Hidden addon whispers, prefix `AFKRP`, from the player to themselves; the server answers the same way.

    client: get\t<name>                    the whole sheet (a bot without one gets it written)
            peek\t<name>                   the title only, never written (tooltips)
            set\t<field>\t<seq>\t<last>\t<text>   one field of your own sheet, in pieces of ~190 bytes
            clear                          delete your sheet
    server: f\t<name>\t<field>\t<seq>\t<last>\t<text>   a field, in pieces
            s\t<name>\t<state>             done | bot | writing | none | gone | failed | busy
            t\t<name>\t<title>             answer to peek

Fields: NA name, TI title, AG age, HO home, AP looks, PE manner, HI history, CU right now. Newlines travel as `~`.

## Names

Bots' memories are keyed by guid; the cleanup that drops them when a guid changes hands compared whole names, so a
surname change (AiPlayerbot.CoaBotSurname, patch `patches/playerbots-bot-surnames.patch`) would have wiped them.
It compares first names now, and `mod_ollama_chat_identity.name` holds the 25 characters of a two-word name.
