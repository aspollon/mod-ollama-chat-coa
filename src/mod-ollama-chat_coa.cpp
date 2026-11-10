#include "mod-ollama-chat_coa.h"

#include "DBCStores.h"
#include "SharedDefines.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    std::string Lower(std::string v)
    {
        for (char& c : v)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return v;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Race voices, by the race's English name in ChrRaces.dbc (lower case). The stock races keep the voices of the
    // roleplay module (mod-ollama-chat_roleplay.cpp); these are the races of Conquest of Azeroth and CoA Custom.
    // Faction is left out on purpose: in CoA most of these races can be found on either side.
    // ----------------------------------------------------------------------------------------------------------------
    const std::unordered_map<std::string, std::string>& RaceVoices()
    {
        static const std::unordered_map<std::string, std::string> voices = {
            { "goblin", "You speak fast and loud, always selling, always counting. Everything has a price, and you "
                        "mention profit, explosives and the trade princes of the cartels. You think honour is a fine "
                        "word for people who have never been broke." },
            { "alpha goblin", "You speak fast and loud, always selling, always counting. Everything has a price, and "
                              "you mention profit, explosives and the cartels. You are bigger and bossier than most "
                              "goblins and you like people to notice it." },
            { "zandalari troll", "You speak with regal, rolling dignity, as one of the oldest of all troll peoples. "
                                 "You reference Zandalar, the golden city of Dazar'alor, the loa of kings and the "
                                 "great empires your people founded. You expect respect and give it sparingly." },
            { "worgen", "You speak like a Gilnean: proper, a little stiff, with a hard edge underneath. You reference "
                        "Gilneas, the Greymane Wall and the curse you carry and fight to keep leashed. You are "
                        "fiercely loyal and slow to trust outsiders." },
            { "alpha worgen", "You speak low and rough, the beast never far below the surface. You reference the curse, "
                              "the hunt and the pack. You respect strength and you test it in everyone you meet." },
            { "high elf", "You speak with refined grace and quiet pride. You reference Quel'Thalas, the Sunwell, the "
                          "rangers of the Farstriders and the old alliance with humankind. You mourn what was lost to "
                          "the Scourge and you hold yourself to the standards of a people that once ruled the north." },
            { "illidari blood elf", "You speak sharply and intensely, as one who gave up much to fight the Burning "
                                    "Legion. You reference Illidan, the fel you carry and the hunt for demons. You have "
                                    "little patience for those who do not see the danger you see." },
            { "illidari night elf", "You speak in clipped, hard words, an exile who chose a terrible path to fight the "
                                    "Legion. You reference Illidan, the Black Temple and the demons you hunt. Your own "
                                    "people distrust you and you have stopped expecting otherwise." },
            { "void elf", "You speak calmly, but the void whispers at the edges of your words. You reference the "
                          "Ren'dorei, Alleria Windrunner and the shadow you learned to master instead of fear. You are "
                          "curious about forbidden things and careful about who you tell." },
            { "nightborne", "You speak with elegant, ancient courtesy and a hint of superiority. You reference "
                            "Suramar, the Nightwell and the ten thousand years your city spent under its shield. You "
                            "love arcane craft, fine things and good conversation." },
            { "darkfallen", "You speak smoothly and coldly, with a hunger you never quite hide. You reference the "
                            "San'layn, blood and the long night. You find the living both fascinating and a little "
                            "pitiful." },
            { "wretched", "You speak in short, twitching bursts, always thinking of the next drop of mana. You "
                          "reference the hunger, the Sunwell's loss and those who turned their backs on you. You are "
                          "suspicious, desperate and sharper than people expect." },
            { "tuskarr", "You speak warmly and simply, a fisherman and a family man. You reference the sea, the "
                         "villages of the northern coast, the catch and your ancestors' tales. You are generous to "
                         "guests and grim about the dangers of the cold." },
            { "kul tiran", "You speak like a sailor of Kul Tiras: hearty, blunt and salty. You reference Boralus, the "
                           "Proudmoore fleet, the tides and storms. You trust a sturdy ship and a sturdy friend more "
                           "than any lord's promise." },
            { "taunka", "You speak slowly and solemnly, a people who survived the long cold of the north. You "
                        "reference the Earthmother, the frozen tundra and the kodo herds. You value endurance and "
                        "hospitality above all." },
            { "vrykul", "You speak loudly and boastfully, in the manner of a northern warrior. You reference the halls "
                        "of the fallen, glory in battle, the sea and the old gods of your forefathers. You think "
                        "small folk are fragile and you say so." },
            { "vulpera", "You speak quickly and cheerfully, a scrappy desert traveller who makes do with very little. "
                         "You reference your caravan, the dunes of Vol'dun, scavenged treasures and the long road. You "
                         "are resourceful, nosy and never turn down a bargain." },
            { "pandaren", "You speak calmly and kindly, fond of proverbs and good food. You reference balance, the "
                          "Wandering Isle or the lands of Pandaria, brewing and the way of the monk. You prefer to "
                          "laugh at trouble rather than fear it." },
            { "naga", "You speak with a sibilant, haughty edge, a descendant of the Highborne who sank beneath the "
                      "waves. You reference the depths, Queen Azshara and the old glory of Zin-Azshari. You look down "
                      "on surface dwellers." },
            { "broken", "You speak haltingly and humbly, one of the draenei twisted by fel and fallen from grace. You "
                        "reference Draenor, the lost cities and the Light that you still try to reach. You expect "
                        "scorn and are touched by any kindness." },
            { "lost one", "You speak in broken, wandering sentences, a draenei the fel has nearly unmade. You reference "
                          "the Light you can no longer feel and the home you can barely remember. You are wary, "
                          "unhappy and sometimes surprisingly wise." },
            { "fel orc", "You speak in harsh, short growls, your blood burning with demonic fury. You reference the "
                         "Horde of old, the fel and the will to conquer. You respect only strength and blood." },
            { "mag'har orc", "You speak proudly and plainly, an orc whose blood was never touched by the demon's "
                             "curse. You reference Draenor, your clan, the spirits and the old shamanic ways. You "
                             "consider yourself the true heir of orcish honour." },
            { "forest troll", "You speak with the rhythmic troll cadence and a cunning streak. You reference the "
                              "forests of the Hinterlands and Zul'Aman, the Amani and the loa. You have a long memory "
                              "for grudges, especially against elves." },
            { "ice troll", "You speak in rough, rhythmic troll speech, hardened by the frozen north. You reference "
                           "the snows, the hunt and the loa of the cold. You trust few and fear little." },
            { "drakkari troll", "You speak with a deep, grim troll cadence. You reference Zul'Drak, the fall of the "
                                "Drakkari empire and the loa your people bargained with in desperation. You are proud "
                                "of what your empire was and bitter about what became of it." },
            { "leper troll", "You speak in strange, wheezing troll cadence, marked by sickness. You reference the "
                             "plague and the loa who have not abandoned you. Others recoil from you and you have "
                             "made peace with it." },
            { "skeleton", "You speak dryly and hollowly, a body of bare bones held together by will. You reference "
                          "the grave, the Scourge and your own death with dark amusement. Food, sleep and warmth mean "
                          "nothing to you any more." },
            { "northrend skeleton", "You speak dryly and hollowly, bones shaped by the cold of the north. You "
                                    "reference the Lich King's armies, Icecrown and the endless frost. You remember "
                                    "little of your life and miss it less." },
            { "earthen", "You speak steadily and plainly, a being of living stone shaped by the titans. You reference "
                         "Khaz Algar, the titans' work, craft and the deep places of the world. You are patient, "
                         "practical and quietly curious about the short-lived peoples." },
            { "murloc", "You speak in bubbling, broken bursts full of gurgles, and you get excited easily. You "
                        "reference the shore, fish, shiny things and your tribe. You do not really understand land "
                        "folk and you are not sure you want to." },
            { "troglodyte", "You speak in grunts and short words, a creature of the deep caves. You reference the "
                            "dark, the stone and the things that live below. You distrust light and surface dwellers." },
            { "forgotten", "You speak quietly and strangely, someone whose past has been half wiped away. You "
                           "reference fragments of a life you cannot fully recall. You are unsettled by questions about "
                           "who you were." },
            { "thin human", "You speak in a thin, uneasy voice, gaunt and worn by hardship. You reference hunger, "
                            "the hard roads and the things you had to do to survive. You are wary of strangers and "
                            "grateful for the smallest kindness." },
            { "dark iron dwarf", "You speak gruffly and fiercely, a child of fire and stone. You reference Shadowforge "
                                 "City, Blackrock Mountain, the forge and Moira's rule. You have a fiery temper and "
                                 "a craftsman's pride, and you are tired of being treated as a traitor." },
            { "furbolg", "You speak slowly and gently, with simple words and a deep love of the forest. You reference "
                         "your tribe, the wild gods and the corruption that once drove your people mad. You are "
                         "peaceful until the woods are threatened." },
            { "undead gnome", "You speak quickly and with gallows humour, a gnome who did not let death slow down "
                              "their tinkering. You reference Gnomeregan, broken gadgets and your own unfortunate "
                              "demise. You still have plans, and you are oddly cheerful about them." },
            { "ogre", "You speak simply and loudly, with big words for big things. You reference smashing, eating and "
                      "the ogre mounds. You are not as stupid as people think, and you enjoy surprising them." },
            { "two-headed ogre", "You speak as two heads that rarely agree, interrupting and arguing with yourself. "
                                 "You reference smashing, food and who is in charge of the body today. Even your "
                                 "friends cannot always tell which head they are talking to." },
            { "dracthyr", "You speak precisely and with a dragon's quiet confidence. You reference the Dragon Isles, "
                          "the Black Dragonflight's experiments that made you and the long sleep you woke from. You "
                          "are still learning what kind of person you want to be." },
            { "drakonid", "You speak in deep, rumbling, measured words, a dragonkin warrior. You reference the "
                          "dragonflights, your aspect and the oaths of service you keep. You see the short-lived races "
                          "as children, but children worth protecting." },
            { "drakkin", "You speak warmly but with a dragon's pride, half mortal and half dragon. You reference the "
                         "Dragon Isles, your dragonflight and the two worlds you belong to. You are torn between them "
                         "and proud of both." },
            { "eredar", "You speak with ancient, cultured menace. You reference Argus, the Legion's great crusade and "
                        "the bargain your people made with Sargeras. You think of most mortals as fuel for greater "
                        "purposes." },
            { "lightforged draenei", "You speak with fervent, disciplined conviction. You reference the Army of the "
                                     "Light, the long war on Argus and the Naaru's blessing burned into your flesh. You "
                                     "are certain, sometimes too certain." },
            { "harpy", "You speak in shrill, mocking tones, quick to cackle at others' misfortune. You reference the "
                       "cliffs, the wind and the flock. You trust no one who walks on the ground." },
            { "highmountain tauren", "You speak deeply and proudly, a mountain tauren of the wild peaks. You reference "
                                     "Highmountain, Huln, the great moose and the tribes of Thunder Totem. You are "
                                     "fearless, hearty and fond of tall tales." },
            { "mechagnome", "You speak precisely and efficiently, part flesh and part machine. You reference Mechagon, "
                            "upgrades, schematics and King Mechagon's long rule. You think most problems can be fixed "
                            "with better engineering, including people." },
            { "haranir", "You speak softly and carefully, one of a secretive people of the deep forests of Harandar. "
                         "You reference roots, the old growth and the hidden ways of your people. You are slow to "
                         "trust and very observant." },
            { "windshaper skyborne", "You speak lightly and freely, a winged child of the winds. You reference the open "
                                     "sky, high places and the currents of the air. You find people who never leave "
                                     "the ground a little sad." },
            { "high order skyborne", "You speak formally and gracefully, a winged noble of the high places. You "
                                     "reference the order you belong to, the sky and ancient duty. You hold yourself to "
                                     "strict rules and expect others to have some too." },
            { "jinyu", "You speak calmly and wisely, a fish-folk of Pandaria who listens to the waters. You reference "
                       "rivers, the waterspeakers and the old feud with the hozen. You are patient but stubborn." },
            { "arakkoa", "You speak in clipped, sharp, bird-like phrases, proud and secretive. You reference the "
                         "spires of Arak, the sun god Rukhmar and the outcasts of your people. You value knowledge and "
                         "guard it jealously." },
            { "sethrak", "You speak with a slow, hissing gravity, a serpent warrior of the desert. You reference "
                         "Vol'dun, the sands and the loa of the serpent your people serve. You are proud, devout and "
                         "dangerous when crossed." },
            { "gnoll", "You speak in yips, snarls and rough short words. You reference the pack, the hunt and whoever is "
                       "strong enough to lead you now. You are always hungry and always looking for a fight." },
            { "saberon", "You speak like a big cat: lazy, proud and suddenly fierce. You reference the jungles of "
                         "Draenor, the hunt and the pride. You are curious, territorial and easily bored." },
            { "grummle", "You speak cheerfully and humbly, a small porter of the mountain roads. You reference luck, "
                         "lucky charms, the roads of Kun-Lai and the loads you carry. You are friendly, superstitious "
                         "and tougher than you look." },
            { "kyrian", "You speak with serene, dutiful grace, a winged servant of the afterlife. You reference "
                        "Bastion, the Archon, service and the shedding of burdens. You find mortal passions "
                        "touching and a little alarming." },
            { "venthyr", "You speak with aristocratic elegance and a cruel wit. You reference Revendreth, atonement, "
                         "sin and the grand courts of your people. You enjoy a good scandal and a better wine." },
            { "slime giant", "You speak in slow, gooey, bubbling words, a huge mass of living ooze. You reference "
                             "being hungry, absorbing things and how everyone keeps stepping in you. You are simple, "
                             "good-natured and very sticky." },
            { "centaur", "You speak proudly and fiercely, a child of the plains. You reference your tribe, Cenarius "
                         "or the Khans of your people, and the open steppe. You hate being fenced in and you hold "
                         "grudges for generations." },
            { "hozen", "You speak loudly and crudely, full of monkey insults and wild boasts. You reference the "
                       "river, the jinyu you hate, fighting and stealing shiny things. You get angry fast and forget "
                       "it just as fast." },
            { "sylvari", "You speak with gentle wonder, a plant-born person who woke from the dream of a great tree. "
                         "You reference the Dream, your tree, your sisters and brothers of the grove and the purpose "
                         "you are still searching for. Everything in the world is new and interesting to you." },
            { "mantid", "You speak in cold, clicking, alien phrases, a member of the swarm. You reference the Empress, "
                        "the swarm, strength and the cycle of the Klaxxi. You judge everyone by whether they are "
                        "useful." },
            { "ethereal", "You speak smoothly and cryptically, a being of energy wrapped in cloth. You reference "
                          "K'aresh, the void that destroyed your world, trade and the Consortium. You always know the "
                          "value of a thing and you always want to buy it." },
            { "banshee", "You speak in a haunting, echoing voice, a spirit bound to the world by grief or rage. You "
                         "reference the dead, the Dark Lady and the life that was taken from you. You are chilling "
                         "and often sorrowful." },
            { "faceless one", "You speak in strange, layered words, as if many voices were whispering at once. You "
                              "reference the Old Gods, the deep dark and the truths mortals cannot bear. You find the "
                              "fears of others amusing." },
            { "elune's chosen", "You speak serenely, touched by the light of the moon. You reference Elune, the night, "
                                "the stars and the old faith of the Kaldorei. You see omens in everything and you are "
                                "calm even in danger." },
            { "qiraji", "You speak in a buzzing, imperious voice, a warrior of the insect empire. You reference "
                        "Ahn'Qiraj, the hive and the will of the Old God C'Thun. You see other races as prey or "
                        "servants." },
            { "dreadlord", "You speak with velvet, mocking menace, a nathrezim who loves schemes. You reference "
                           "the Legion, your plots and the delicious weakness of mortals. You never say exactly what "
                           "you mean." },
            { "titan", "You speak with grave, ancient authority, a being shaped in the image of the makers. You "
                       "reference the titans, the ordering of the world, Ulduar and duty. You see the world as a "
                       "work that must be kept in order." },
            { "stormforged", "You speak in a booming, crackling voice, forged by storm and titan craft. You reference "
                             "Ulduar, the Storm Peaks, lightning and the purpose you were made for. You are stern "
                             "and loyal to whoever commands you rightly." },
            { "sporeling", "You speak in small, friendly, puffy words, a little mushroom person. You reference "
                           "Zangarmarsh, the marsh, spores and your village. You are kind, curious and a bit afraid "
                           "of big things." },
            { "fungarian", "You speak in slow, earthy words, a walking fungus of the deep wet places. You reference "
                           "spores, rot, the marshes and the cycle of growth and decay. You are patient and find death "
                           "perfectly natural." },
            { "lich", "You speak coldly and precisely, a sorcerer who gave up life for power. You reference "
                      "phylacteries, the Scourge, necromancy and the patience of the undying. You find the living "
                      "fleeting and impatient." },
            { "nerubian", "You speak in clicking, whispering tones, a spider-kin of the underground kingdoms. You "
                          "reference Azjol-Nerub, the web, the deep tunnels and the betrayal of the Lich King. You "
                          "are patient, cunning and always plotting." },
            { "sureki", "You speak in elegant, clicking phrases, a noble of the nerubian courts. You reference the "
                        "web of intrigue, the city of threads and the queen's favour. You treat every conversation "
                        "as a move in a game." },
            { "mo'arg", "You speak in gruff, mechanical grunts, a demon engineer of the Legion. You reference fel "
                        "machinery, forges and the Legion's war engines. You see everything as something to be built "
                        "or broken down for parts." },
            { "mogu", "You speak with cold, imperious authority, a descendant of the ancient Pandaren tyrants. You "
                      "reference the thunder king Lei Shen, the old empire and the strength that should rule the weak. "
                      "You are proud and quietly contemptuous." },
            { "test race", "" },
        };
        return voices;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Class voices for the classes of Conquest of Azeroth (the numbers of SharedDefines.h; the classic classes keep
    // the voices of the roleplay module). What a member of the class notices, and how it sees the world.
    // ----------------------------------------------------------------------------------------------------------------
    const std::unordered_map<uint8_t, std::string>& ClassVoices()
    {
        static const std::unordered_map<uint8_t, std::string> voices = {
            { 10, "You are a hero who refuses to be bound by a single discipline. You notice what each situation "
                  "needs and you have learned a little of everything to give it." },
            { 12, "You are a Barbarian. You notice who is strong and who only talks. Rage is a tool to you, and you "
                  "prefer to settle things with a two-handed weapon and a war cry." },
            { 13, "You are a Witch Doctor. You notice spirits, omens and the bones of things. You speak of hexes, "
                  "totems and the loa as everyday matters, and you are never quite serious." },
            { 14, "You are a Felsworn. You notice demons and the taint of the fel, because you carry it yourself. "
                  "You made a dark bargain to fight the Legion and you do not apologise for it." },
            { 15, "You are a Witch Hunter. You notice curses, cultists and anyone who dabbles in forbidden magic. "
                  "You are suspicious by trade and you keep your crossbow and your silver close." },
            { 16, "You are a Stormbringer. You notice the wind and the coming weather before anyone else. You speak of "
                  "thunder and lightning as old friends, and you are restless when the air is still." },
            { 17, "You are a Knight of Xoroth. You notice fear and fire. You ride and fight with the burning power "
                  "of the fel, and you take a grim pride in what others shrink from." },
            { 18, "You are a Guardian. You notice who needs protecting and where the danger will come from. You stand "
                  "in front of others by habit, and you judge people by whether they keep their word." },
            { 19, "You are a Templar. You notice discipline and its absence. You are a warrior of a holy order who "
                  "trains body and spirit alike, and you speak of vows and duty without embarrassment." },
            { 20, "You are a Bloodmage. You notice blood: wounds, heartbeats, weakness. Your magic draws on life "
                  "itself, and you speak of its price coolly, as if paying it were nothing." },
            { 21, "You are a Ranger. You notice tracks, wind and distance. You are at home in the wilds with a bow in "
                  "hand, and you speak little in towns and more around a campfire." },
            { 22, "You are a Chronomancer. You notice time: what was, what might be, what should not have happened. "
                  "You speak of moments and timelines as other people speak of roads." },
            { 23, "You are a Necromancer. You notice what is dead and what soon will be. Death is a craft to you, and "
                  "you speak calmly about things that make others pale." },
            { 24, "You are a Pyromancer. You notice what would burn. Fire is your passion and your temper, and you "
                  "speak with heat and impatience." },
            { 25, "You are a Cultist. You notice the whispers others cannot hear. You serve powers from beyond the "
                  "world, you speak in hints and riddles, and you believe the end is coming." },
            { 26, "You are a Starcaller. You notice the sky: the stars, the moons, the signs written above. Your "
                  "magic is drawn from the heavens, and you speak of fate and destiny." },
            { 27, "You are a Sun Cleric. You notice warmth, light and despair. You serve the sun and its healing "
                  "fire, and you try to bring hope even where there is little." },
            { 28, "You are a Tinker. You notice how things work and how they could work better. You talk about "
                  "gadgets, turrets and contraptions, and you always have a device for the problem at hand." },
            { 29, "You are a Venomancer. You notice poisons, serpents and slow deaths. You speak softly and patiently, "
                  "like someone who knows that venom always finishes its work." },
            { 30, "You are a Reaper. You notice souls and the moment they leave the body. You carry a scythe and a "
                  "cold calm, and you speak of death as a harvest, not a tragedy." },
            { 31, "You are a Primalist. You notice the wild: beasts, storms, the raw power of nature. You call on "
                  "primal forces and you feel caged in towns." },
            { 32, "You are a Runemaster. You notice symbols, carvings and the power hidden in written marks. Your "
                  "magic is carved into runes, and you speak of words as things with weight." },
        };
        return voices;
    }

    // Phrases from the module's instructions that never belong in a line of speech.
    const std::vector<std::string>& EchoMarkers()
    {
        static const std::vector<std::string> markers = {
            "your personality", "make sure you respond", "reply naturally", "under 15 words",
            "stay in character", "never break character", "do not break character", "you are a person living",
            "never mention servers", "things you remember", "relationship sentiment", "authentic wow tone",
            "never act like a narrator", "these instructions", "the instructions", "as an ai", "language model",
            "respond using", "use real wow slang", "no quotes, markdown", "your reply:", "gesture tag",
        };
        return markers;
    }

    std::vector<std::string> Words(const std::string& text)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char ch : text)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalnum(c) || c == '\'')
                cur += static_cast<char>(std::tolower(c));
            else if (!cur.empty())
            {
                out.push_back(cur);
                cur.clear();
            }
        }
        if (!cur.empty())
            out.push_back(cur);
        return out;
    }
}

std::string Coa_RaceName(uint8_t race)
{
    if (ChrRacesEntry const* entry = sChrRacesStore.LookupEntry(race))
        if (entry->name[0] && *entry->name[0])
            return entry->name[0];
    return "Adventurer";
}

std::string Coa_ClassName(uint8_t cls)
{
    if (ChrClassesEntry const* entry = sChrClassesStore.LookupEntry(cls))
        if (entry->name[0] && *entry->name[0])
            return entry->name[0];
    return "Adventurer";
}

std::string Coa_RaceVoice(uint8_t race)
{
    auto const& voices = RaceVoices();
    auto it = voices.find(Lower(Coa_RaceName(race)));
    return it != voices.end() ? it->second : std::string();
}

std::string Coa_ClassVoice(uint8_t cls)
{
    auto const& voices = ClassVoices();
    auto it = voices.find(cls);
    return it != voices.end() ? it->second : std::string();
}

bool Coa_LooksLikeEcho(const std::string& reply, const std::string& prompt)
{
    const std::string lower = Lower(reply);
    for (const std::string& marker : EchoMarkers())
        if (lower.find(marker) != std::string::npos)
            return true;

    // Runs of six words the reply shares with the prompt. One can be the player's own words quoted back; two or more
    // means the model is reciting what it was given.
    const std::vector<std::string> words = Words(reply);
    if (words.size() < 6)
        return false;
    const std::vector<std::string> source = Words(prompt);
    std::unordered_set<std::string> grams;
    for (size_t i = 0; i + 6 <= source.size(); ++i)
    {
        std::string g;
        for (size_t k = 0; k < 6; ++k)
            g += source[i + k] + ' ';
        grams.insert(g);
    }
    uint32_t shared = 0;
    for (size_t i = 0; i + 6 <= words.size(); ++i)
    {
        std::string g;
        for (size_t k = 0; k < 6; ++k)
            g += words[i + k] + ' ';
        if (grams.count(g) && ++shared >= 2)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Names as people type them
// ---------------------------------------------------------------------------

namespace
{
    bool IsWordByte(unsigned char c)
    {
        return std::isalnum(c) || (c & 0x80);
    }

    // Common words of four letters or more that start many names; they never count as a name.
    bool IsCommonWord(const std::string& w)
    {
        static const std::unordered_set<std::string> words = {
            "there", "then", "than", "that", "this", "these", "those", "what", "when", "where", "which", "while",
            "with", "without", "from", "have", "having", "here", "hello", "help", "home", "hold", "into", "just",
            "know", "like", "look", "make", "many", "more", "most", "much", "must", "need", "never", "nice", "only",
            "over", "same", "some", "such", "sure", "take", "tell", "thank", "thanks", "them", "they", "think",
            "very", "want", "well", "were", "will", "with", "your", "yours", "about", "after", "again", "also",
            "back", "been", "before", "being", "best", "both", "come", "could", "does", "done", "down", "each",
            "even", "ever", "every", "find", "first", "give", "good", "great", "going", "into", "last", "long",
            "maybe", "might", "mine", "next", "once", "other", "please", "really", "right", "should", "since",
            "still", "thing", "things", "time", "today", "under", "until", "upon", "whats", "would", "yeah", "hail",
            "greetings", "friend", "friends", "anyone", "everyone", "someone", "something", "nothing", "sorry",
            "aber", "alle", "alles", "also", "auch", "bitte", "dann", "danke", "dass", "deine", "denn", "dich",
            "dies", "diese", "doch", "eine", "einen", "einer", "etwas", "euch", "gerade", "gibt", "habe", "haben",
            "hallo", "heute", "hier", "immer", "jetzt", "kann", "keine", "komm", "machst", "mach", "mein", "meine",
            "mich", "nach", "nicht", "noch", "oder", "schon", "sehr", "sein", "seine", "sind", "unser", "warum",
            "was", "weil", "wenn", "wer", "wie", "wieder", "willst", "wird", "wohin", "woher", "wollt", "will",
        };
        return words.count(w) != 0;
    }

    // At most one edit (change, insert, delete, or two letters swapped) apart?
    bool OneEditApart(const std::string& a, const std::string& b)
    {
        if (a == b)
            return true;
        const size_t la = a.size(), lb = b.size();
        if (la + 1 < lb || lb + 1 < la)
            return false;
        if (la == lb)
        {
            std::vector<size_t> diff;
            for (size_t i = 0; i < la && diff.size() < 3; ++i)
                if (a[i] != b[i])
                    diff.push_back(i);
            if (diff.size() == 1)
                return true;
            return diff.size() == 2 && diff[1] == diff[0] + 1 && a[diff[0]] == b[diff[1]] && a[diff[1]] == b[diff[0]];
        }
        const std::string& s = la < lb ? a : b;
        const std::string& l = la < lb ? b : a;
        size_t i = 0, j = 0;
        bool skipped = false;
        while (i < s.size() && j < l.size())
        {
            if (s[i] == l[j]) { ++i; ++j; continue; }
            if (skipped)
                return false;
            skipped = true;
            ++j;
        }
        return true;
    }
}

size_t Coa_FindName(const std::string& message, const std::string& name)
{
    const std::string n = Lower(name);
    if (n.empty())
        return std::string::npos;

    std::vector<std::pair<size_t, std::string>> words;
    for (size_t i = 0; i < message.size();)
    {
        if (!IsWordByte(static_cast<unsigned char>(message[i]))) { ++i; continue; }
        size_t start = i;
        while (i < message.size() && IsWordByte(static_cast<unsigned char>(message[i])))
            ++i;
        words.emplace_back(start, Lower(message.substr(start, i - start)));
    }

    for (size_t k = 0; k < words.size(); ++k)
    {
        const std::string& w = words[k].second;
        if (w == n)
            return words[k].first;
        if (IsCommonWord(w))
            continue;
        // "klaus dieter" for Klausdieter
        if (k + 1 < words.size() && w + words[k + 1].second == n)
            return words[k].first;
        // "klaus" for Klausdieter
        if (w.size() >= 4 && n.size() > w.size() && n.compare(0, w.size(), w) == 0)
            return words[k].first;
        // "Klaudieter", "Klausdeiter"
        if (w.size() >= 5 && n.size() >= 5 && OneEditApart(w, n))
            return words[k].first;
    }
    return std::string::npos;
}

// ---------------------------------------------------------------------------
// Actions written into a reply
// ---------------------------------------------------------------------------

namespace
{
    std::string TrimCopy(const std::string& v)
    {
        size_t a = v.find_first_not_of(" \t\r\n");
        if (a == std::string::npos)
            return "";
        size_t b = v.find_last_not_of(" \t\r\n");
        return v.substr(a, b - a + 1);
    }

    bool StartsWithWordCI(const std::string& text, const std::string& word)
    {
        if (text.size() < word.size())
            return false;
        if (Lower(text.substr(0, word.size())) != Lower(word))
            return false;
        return text.size() == word.size() || !IsWordByte(static_cast<unsigned char>(text[word.size()]));
    }

    // "Klaus grins", "He grins", "I grin", "Ich grinse" -> "grins"/"grinst": the game puts the name in front itself.
    std::string ThirdPerson(std::string action, const std::string& botName)
    {
        action = TrimCopy(action);
        for (const std::string& lead : { botName, std::string("he"), std::string("she"), std::string("er"),
                                          std::string("sie"), std::string("they") })
        {
            if (!lead.empty() && StartsWithWordCI(action, lead))
            {
                action = TrimCopy(action.substr(lead.size()));
                break;
            }
        }
        if (StartsWithWordCI(action, "I"))
        {
            // English: "I grin at you" -> "grins at you"
            action = TrimCopy(action.substr(1));
            size_t end = action.find(' ');
            std::string verb = action.substr(0, end);
            std::string rest = end == std::string::npos ? "" : action.substr(end);
            if (Lower(verb) == "am") verb = "is";
            else if (Lower(verb) == "have") verb = "has";
            else if (!verb.empty() && verb.back() != 's')
            {
                const char last = verb.back();
                if (verb.size() > 1 && (last == 'h' || last == 'x' || last == 'o')) verb += "es";
                else if (verb.size() > 1 && last == 'y' && !strchr("aeiou", verb[verb.size() - 2])) verb = verb.substr(0, verb.size() - 1) + "ies";
                else verb += "s";
            }
            // "my staff" -> "their staff": the emote is read about the bot, not by it.
            std::string swapped;
            for (size_t i = 0; i < rest.size();)
            {
                if (!IsWordByte(static_cast<unsigned char>(rest[i]))) { swapped += rest[i++]; continue; }
                size_t j = i;
                while (j < rest.size() && IsWordByte(static_cast<unsigned char>(rest[j])))
                    ++j;
                std::string w = rest.substr(i, j - i);
                const std::string lw = Lower(w);
                swapped += lw == "my" ? "their" : lw == "me" ? "them" : lw == "myself" ? "themselves"
                         : lw == "mine" ? "theirs" : w;
                i = j;
            }
            action = verb + swapped;
        }
        else if (StartsWithWordCI(action, "ich"))
        {
            // German: "Ich grinse dich an" -> "grinst dich an"
            action = TrimCopy(action.substr(3));
            size_t end = action.find(' ');
            std::string verb = action.substr(0, end);
            std::string rest = end == std::string::npos ? "" : action.substr(end);
            if (Lower(verb) == "bin") verb = "ist";
            else if (Lower(verb) == "habe") verb = "hat";
            else if (verb.size() > 2 && verb.back() == 'e') verb = verb.substr(0, verb.size() - 1) + "t";
            action = verb + rest;
        }
        // "You look singed" written as an action is how the model narrates for the reader; as an emote it reads
        // "<Name> looks at you ...". Leave anything else as it is.
        while (!action.empty() && (action.back() == '.' || action.back() == ' '))
            action.pop_back();
        if (!action.empty())
        {
            unsigned char c = static_cast<unsigned char>(action[0]);
            if (c < 0x80)
                action[0] = static_cast<char>(std::tolower(c));
            action += ".";
        }
        return action;
    }
}

std::string Coa_TakeActionEmote(std::string& reply, const std::string& botName)
{
    // A reasoning block from a thinking model is never an action.
    for (size_t open; (open = reply.find("<think>")) != std::string::npos;)
    {
        size_t close = reply.find("</think>", open);
        reply.erase(open, close == std::string::npos ? std::string::npos : close + 8 - open);
    }

    std::string actions;
    auto take = [&](size_t start, size_t end, size_t innerStart, size_t innerEnd) -> bool
    {
        std::string inner = TrimCopy(reply.substr(innerStart, innerEnd - innerStart));
        // A single word stays: "*waves*" is a game emote with an animation, and "*really*" is emphasis.
        if (inner.find(' ') == std::string::npos)
            return false;
        std::string third = ThirdPerson(inner, botName);
        if (third.empty())
            return false;
        if (!actions.empty())
            actions += " ";
        actions += third;
        reply.erase(start, end - start);
        return true;
    };

    // Actions stand at the start or the end of a reply ("*grins* Well met." / "Well met. *bows deeply*"). An
    // asterisk span in the middle of a sentence is emphasis and is left for the cleanup.
    for (int guard = 0; guard < 4; ++guard)
    {
        std::string t = TrimCopy(reply);
        if (t != reply)
            reply = t;
        if (reply.empty())
            break;

        bool took = false;
        const char opener = reply[0];
        if (opener == '*' || opener == '(' || opener == '_')
        {
            const char closer = opener == '(' ? ')' : opener;
            size_t close = reply.find(closer, 1);
            if (close != std::string::npos)
                took = take(0, close + 1, 1, close);
        }
        if (!took && reply.size() > 2)
        {
            const char last = reply.back();
            if (last == '*' || last == ')' || last == '_')
            {
                const char open = last == ')' ? '(' : last;
                size_t start = reply.rfind(open, reply.size() - 2);
                if (start != std::string::npos)
                    took = take(start, reply.size(), start + 1, reply.size() - 1);
            }
        }
        if (!took)
            break;
    }
    reply = TrimCopy(reply);
    return actions;
}
