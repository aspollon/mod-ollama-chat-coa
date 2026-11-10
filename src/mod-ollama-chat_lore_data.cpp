// Generated from the lore tables of mod-ollama-chat-coa: what the bots know of Azeroth.
// Races by their English name in ChrRaces.dbc (lower case), classes by id, zones by zone id, topics by keywords.
// Keywords are lower-case phrases, separated by commas; a message containing one brings the entry up.

#include "mod-ollama-chat_lore.h"

namespace LoreData
{

const LoreEntry Races[] = {
    { "human", 0, "human,humans,mankind,stormwind,lordaeron,alliance of lordaeron",
        "Humans descend from the Arathi of Strom and built the seven kingdoms. Lordaeron fell to the Scourge "
        "in the Third War when Prince Arthas murdered his father; Stormwind was razed by the orcish Horde in "
        "the First War and rebuilt. King Varian Wrynn rules Stormwind now and leads the Alliance against the "
        "Lich King." },
    { "orc", 0, "orc,orcs,orcish,durotar,horde",
        "The orcs came from Draenor through the Dark Portal, driven by the demon blood their warlocks drank "
        "for the Burning Legion. Beaten, they rotted in internment camps until Thrall freed them, broke the "
        "bloodlust and led them across the sea to Kalimdor, where they founded Durotar and Orgrimmar. Honour "
        "and redemption matter deeply to them." },
    { "dwarf", 0, "dwarf,dwarves,dwarven,ironforge,bronzebeard,khaz modan",
        "The dwarves of Khaz Modan carve their halls deep in the mountains; Ironforge is their capital under "
        "King Magni Bronzebeard. The War of the Three Hammers split them into Bronzebeard, Wildhammer and "
        "Dark Iron clans. The Explorers' League digs for proof that the titans made dwarves from stone, and "
        "they love ale, craft and a good brawl." },
    { "night elf", 0, "night elf,night elves,kaldorei,darnassus,teldrassil,elune",
        "The kaldorei are the oldest elves, worshippers of Elune. Their queen Azshara's hunger for magic "
        "brought the Burning Legion in the War of the Ancients ten thousand years ago and sundered the world."
        " Since then they shunned arcane magic, slept in the Emerald Dream as druids, and guarded the "
        "forests. They planted Teldrassil after losing their immortality." },
    { "undead", 0, "undead,forsaken,undercity,sylvanas,lordaeron",
        "The Forsaken are dead of Lordaeron who broke free of the Lich King's will when his power waned. Led "
        "by Sylvanas Windrunner, the Banshee Queen, they built the Undercity beneath the ruins of Lordaeron's"
        " capital. The living fear and despise them, so they joined the Horde out of need. Their Royal "
        "Apothecary Society brews a plague in secret." },
    { "tauren", 0, "tauren,thunder bluff,mulgore,earthmother,shu'halo",
        "The tauren, the shu'halo, are nomads of the plains who worship the Earthmother, An'she the sun and "
        "Mu'sha the moon. Hunted by centaur, they were helped by Thrall and repaid him by joining the Horde. "
        "Cairne Bloodhoof united the tribes in Mulgore and built Thunder Bluff on its mesas. They are calm, "
        "wise and fearsome when roused." },
    { "gnome", 0, "gnome,gnomes,gnomish,gnomeregan,tinker town,mekkatorque",
        "The gnomes are brilliant inventors whose city Gnomeregan fell when the troggs rose from below; on "
        "the advice of the traitor Mekgineer Thermaplugg they flooded it with radiation, which killed many of"
        " their own. The survivors live in Tinker Town in Ironforge under High Tinker Mekkatorque and dream "
        "of taking their home back. They are cheerful, curious and endlessly building." },
    { "troll", 0, "troll,trolls,darkspear,echo isles,sen'jin,vol'jin,loa",
        "The Darkspear trolls were a small tribe driven from their islands by murlocs and sea witches before "
        "Thrall rescued them. Vol'jin, son of Sen'jin, swore them to the Horde, and they live in Durotar and "
        "the Echo Isles. Trolls once ruled vast empires like Zul'Aman and Gurubashi and worship the loa, the "
        "spirits of the wild." },
    { "blood elf", 0, "blood elf,blood elves,sin'dorei,silvermoon,quel'thalas,sunwell",
        "The sin'dorei are high elves who renamed themselves for the blood of their people spilled when "
        "Arthas marched through Quel'Thalas, killed King Anasterian and defiled the Sunwell. Cut off from its"
        " magic they suffer a hunger for mana. Prince Kael'thas led many to Outland; Lor'themar Theron rules "
        "Silvermoon now, and they joined the Horde." },
    { "draenei", 0, "draenei,exodar,velen,naaru,eredar,argus",
        "The draenei are eredar who fled Argus with the prophet Velen when Sargeras seduced their people into"
        " the Burning Legion. For twenty-five thousand years they ran, guided by the naaru and the Light, "
        "until they settled on Draenor, where the orcs slaughtered them. Their ship, the Exodar, crashed on "
        "the Azuremyst Isles, and they joined the Alliance." },
    { "goblin", 0, "goblin,goblins,steamwheedle,cartel,bilgewater,kezan,undermine",
        "Goblins were once slaves of the jungle trolls on Kezan until kaja'mite made them clever. Now trade "
        "princes run cartels like Steamwheedle, which keeps neutral ports at Booty Bay, Gadgetzan, Ratchet "
        "and Everlook. They sell to anyone, build explosive machines and believe that every war is a business"
        " opportunity." },
    { "alpha goblin", 0, "alpha goblin",
        "Alpha goblins are the bigger, tougher bruisers of the cartels: the muscle that collects debts and "
        "keeps the trade princes' rivals honest. Most goblins think in coin; these think in coin and fists. "
        "On Kezan the toughest rose to run their own crews, and many hire out as guards and enforcers." },
    { "zandalari troll", 0, "zandalari,zandalar,dazar'alor,zuldazar,rastakhan",
        "The Zandalari are the eldest of the trolls, from the island of Zandalar, where King Rastakhan rules "
        "from the golden pyramid of Dazar'alor. Every troll empire of old - Gurubashi, Amani, Drakkari - grew"
        " from Zandalari roots. Their priests keep the oldest knowledge of the loa, and they fear the slow "
        "fading of trollkind." },
    { "worgen", 0, "worgen,gilneas,gilnean,greymane,curse of the worgen",
        "The worgen are Gilneans struck by a curse that began with Archmage Arugal, who summoned wolf-beasts "
        "from another world to fight the Scourge. Gilneas had walled itself off behind the Greymane Wall "
        "under King Genn Greymane, and the curse spread inside it. Druids taught many to keep their minds "
        "while wearing the beast's shape." },
    { "alpha worgen", 0, "alpha worgen,pack leader",
        "Alpha worgen are those in whom the curse runs strongest: bigger, wilder, closer to the beast. Some "
        "lead packs of feral worgen in Silverpine and Duskwood; others fight beside the Gilneans with barely "
        "held rage. Ordinary worgen respect and fear them in equal measure." },
    { "high elf", 0, "high elf,high elves,quel'dorei,farstriders,quel'thalas,silver covenant",
        "The quel'dorei are the elves who left Kalimdor and founded Quel'Thalas around the Sunwell. They "
        "fought beside humans in the Troll Wars and the Second War, gave humanity its first mages and lost "
        "almost everything when Arthas came. Those who kept the old name stay loyal to the Alliance, many in "
        "Dalaran's Silver Covenant." },
    { "illidari blood elf", 0, "illidari,illidan,black temple,demon hunter",
        "The Illidari are elves who followed Illidan Stormrage, the Betrayer, into Outland. They let him bind"
        " demons into their bodies and carved fel tattoos into their skin to see the Legion and hunt it. "
        "Kael'thas' blood elves joined Illidan at the Black Temple. Most of the world calls them traitors; "
        "they call themselves the only ones willing to pay the price." },
    { "illidari night elf", 0, "illidari,illidan,black temple,demon hunter,mardum",
        "Illidari night elves are the first demon hunters, trained by Illidan Stormrage after the War of the "
        "Ancients. Their own people imprisoned or exiled them for using demonic power. They sacrificed their "
        "eyes for spectral sight and live only to destroy the Burning Legion, whatever it costs them." },
    { "void elf", 0, "void elf,void elves,ren'dorei,alleria,umbric",
        "The ren'dorei are elves of Silvermoon who studied the void under Magister Umbric and were exiled for"
        " it. When the shadow nearly consumed them, Alleria Windrunner, who had learned to master the void "
        "herself, showed them how to control it. They now serve the Alliance, carrying the whispers within "
        "and refusing to give in to them." },
    { "nightborne", 0, "nightborne,shal'dorei,suramar,nightwell,elisande",
        "The nightborne are Highborne elves of Suramar who sealed their city under a great shield when the "
        "Well of Eternity was destroyed. Ten thousand years they lived on the arcane waters of the Nightwell,"
        " hidden from the world, ruled by Grand Magistrix Elisande. Cut off from it they wither into the "
        "Withered. They love the arcane, art and ancient courtesy." },
    { "darkfallen", 0, "darkfallen,san'layn,vampire,blood prince,icecrown",
        "The darkfallen, the san'layn, are elves raised by the Lich King after the fall of Quel'Thalas. They "
        "feed on blood and serve the Scourge as its princes - Valanar, Keleseth and Taldaram in Icecrown "
        "Citadel, with Blood-Queen Lana'thel above them. A few broke free of the Lich King, but the hunger "
        "never leaves them." },
    { "wretched", 0, "wretched,mana addict,withered",
        "The Wretched are elves who gave in completely to the hunger for magic after the Sunwell fell. They "
        "drain any source of mana they can find and have lost the dignity of their people. Silvermoon chased "
        "them out; they haunt the ruins of Eversong and the Ghostlands, and their own kin treat them as a "
        "warning." },
    { "tuskarr", 0, "tuskarr,kaskala,kamagua,moa'ki,unu'pe,northrend coast",
        "The tuskarr are a walrus people of Northrend's coasts. They live in fishing villages like Kaskala, "
        "Unu'pe, Moa'ki and Kamagua, hunt whales and fish, and honour their ancestors with long tales. The "
        "Kvaldir and Scourge raid their villages and the nerubians attack from below, but they welcome any "
        "honest visitor." },
    { "kul tiran", 0, "kul tiran,kul tirans,kul tiras,boralus,proudmoore,daelin",
        "Kul Tiras is an island nation of sailors whose fleet was the greatest of the human kingdoms. Admiral"
        " Daelin Proudmoore died at Theramore fighting the Horde, a death Kul Tiras never forgot or forgave; "
        "his daughter Jaina had sided with Thrall. Kul Tirans are big, hardy people who trust the sea, their "
        "family and their ship." },
    { "taunka", 0, "taunka,camp taunka'le,warsong hold,vesh'kar",
        "The taunka are cousins of the tauren who survived the cold of Northrend. They wandered the tundra "
        "with their kodo for ages, hunted by Scourge and nerubians, and when the Horde arrived they joined "
        "it, sharing the Earthmother faith. Their camps stand in the Borean Tundra, the Howling Fjord and "
        "Dragonblight." },
    { "vrykul", 0, "vrykul,utgarde,ymiron,valkyr,kvaldir,frost giants",
        "The vrykul are giant warriors of Northrend, made by the titans and cursed with flesh. King Ymiron "
        "rules from Utgarde, and many clans now serve the Lich King, who promised them glory and sent his "
        "val'kyr to raise their dead. They worship strength and the halls of the fallen; their ships raid the"
        " coasts." },
    { "vulpera", 0, "vulpera,vol'dun,caravan",
        "The vulpera are small fox people of the deserts of Vol'dun, wandering in caravans. They scavenge, "
        "trade, tame beasts and make clever tools from what others throw away. The Sethrak and the desert "
        "itself are their enemies, and they treasure family, stories and anything shiny they find on the "
        "road." },
    { "pandaren", 0, "pandaren,pandaria,wandering isle,shen-zin su,chen stormstout,brew",
        "The pandaren are a peaceful people from the hidden land of Pandaria, wrapped in mists since the "
        "Sundering, and from the Wandering Isle, a great turtle that carries pandaren across the seas. Chen "
        "Stormstout fought beside Thrall and Rexxar. They follow the way of balance: brewing, cooking, "
        "martial arts and laughter." },
    { "naga", 0, "naga,azshara,nazjatar,highborne,lady vashj,serpentshrine",
        "The naga are the Highborne who sank with Zin-Azshari when the Well of Eternity imploded. Queen "
        "Azshara bargained with the Old Gods and her people were changed into serpents of the deep. Lady "
        "Vashj served Illidan in Zangarmarsh; the naga raid the coasts and look down on surface dwellers." },
    { "broken", 0, "broken,krokul,draenei of draenor",
        "The Broken are draenei whom the fel magic of Draenor's war twisted and diminished. They lost their "
        "bond with the naaru and most of their old wisdom, and were left to scrape by in Outland. Akama's "
        "Ashtongue and the Broken of Telaar fight their own battles; full draenei pity them, and few of them "
        "can feel the Light." },
    { "lost one", 0, "lost one,lost ones",
        "The Lost Ones are draenei the fel corrupted beyond even the Broken. Many have nearly lost their "
        "minds and their memory of Argus and the Light. They live in ruins and marshes of Outland, "
        "mistrustful and wretched, though now and then a Lost One remembers something of the wisdom of their "
        "people." },
    { "fel orc", 0, "fel orc,fel orcs,fel horde,hellfire,kargath,magtheridon",
        "Fel orcs are orcs who drank demon blood again in Outland. The pit lord Magtheridon and later "
        "Illidan's Fel Horde created them in Hellfire Citadel. Their skin is red, their minds burn with fury "
        "and their loyalty belongs to whoever feeds them fel. Orcs of the Horde see in them the curse they "
        "fought so hard to break." },
    { "mag'har orc", 0, "mag'har,mag'har orc,garadar,greatmother geyah,nagrand",
        "The Mag'har are orcs of Outland who never drank the demon blood. They lived hidden in Nagrand, led "
        "by Garrosh Hellscream and the Greatmother Geyah, until Thrall found them. Uncorrupted, they keep the"
        " old shamanic ways, and many of them consider themselves the truest orcs." },
    { "forest troll", 0, "forest troll,forest trolls,amani,zul'aman,hinterlands,zul'jin,revantusk",
        "The forest trolls of the Amani empire ruled the Eastern Kingdoms before the elves came, and fought "
        "them for thousands of years. Zul'jin led them in the Second War; Zul'Aman still stands in the "
        "eastern forests. Some tribes, like the Revantusk of the Hinterlands, join the Horde; most still hate"
        " elves and humans." },
    { "ice troll", 0, "ice troll,ice trolls,frostmane,winterax,drakkari",
        "Ice trolls live in the snows of Alterac, Dun Morogh and Northrend. Tribes like the Frostmane and the"
        " Winterax are fierce hunters and raiders, cousins of the Drakkari. They have little contact with the"
        " other races except by spear, and the cold has made them hard." },
    { "drakkari troll", 0, "drakkari,zul'drak,gundrak,drak'tharon",
        "The Drakkari empire of Zul'Drak was the greatest troll empire of the north. Under the Scourge and "
        "the nerubians it crumbled, and in desperation the Drakkari began sacrificing their own loa to drink "
        "their power. Gundrak and Drak'Tharon still stand; the Argent Crusade and the Ebon Blade fight there." },
    { "leper troll", 0, "leper troll,plague troll",
        "Leper trolls are trolls the plague ate alive without killing them. Their bodies rot and wheeze, but "
        "their spirit endures, and they have found loa who did not abandon them. Other trolls shun them, and "
        "they have made their own villages in the wastes." },
    { "skeleton", 0, "skeleton,skeletons,bones,risen",
        "Skeletons are dead whom necromancy raised without flesh. Most serve the Scourge as mindless "
        "soldiers; a few kept their will when the Lich King's grip weakened. They need no food, warmth or "
        "sleep and remember their living days only dimly." },
    { "northrend skeleton", 0, "northrend skeleton,icecrown,scourge",
        "The skeletons of Northrend were raised from the dead of the north: vrykul, nerubian victims and "
        "fallen soldiers of the Lich King's war. The cold preserves their bones, and Icecrown sends them in "
        "endless legions. Those who broke free wander the wastes, half remembering who they were." },
    { "earthen", 0, "earthen,khaz algar,ulduar,titans,stone",
        "The earthen are beings of living stone made by the titans to shape the world's deep places. The "
        "curse of flesh turned many into the first dwarves; the rest slept in stone vaults or served in "
        "Ulduar. They are patient craftsmen and keepers who remember the titans' work." },
    { "murloc", 0, "murloc,murlocs,mrgl",
        "Murlocs are amphibious fish people found on every shore of Azeroth. They live in tribes, build huts "
        "of reeds and shells, hunt fish and attack anyone who comes too close. Few outsiders understand their"
        " gurgling language, and most consider them pests. Some murlocs, like those of Winterfin in "
        "Northrend, are surprisingly friendly." },
    { "troglodyte", 0, "troglodyte,trogg,troggs,uldaman",
        "The troggs are stone creatures from the depths, flawed failures of the titans' work sealed beneath "
        "Uldaman. Released by dwarven digging, they overran Gnomeregan and the tunnels of Khaz Modan. They "
        "are brutal, stupid and many, and dwarves and gnomes hate them." },
    { "forgotten", 0, "forgotten",
        "The Forgotten are dead who were neither taken by the Scourge nor accepted among the Forsaken. They "
        "drift between, nameless and remembered by no one, finding their own way. They see the world with the"
        " detachment of those who have already lost everything." },
    { "thin human", 0, "thin human,gaunt",
        "These gaunt humans come from the poorest corners of the kingdoms: refugees of Lordaeron, the starved"
        " of Westfall, the plague-haunted villages of the north. Life gave them nothing, and they learned to "
        "get by on little, to trust little and to keep going." },
    { "dark iron dwarf", 0, "dark iron,dark iron dwarf,dark iron dwarves,thaurissan,blackrock depths,ragnaros,moira",
        "The Dark Iron dwarves lost the War of the Three Hammers; their sorcerer-king Thaurissan summoned "
        "Ragnaros the Firelord in revenge and the eruption made the Burning Steppes and Searing Gorge. They "
        "lived as slaves of the Firelord beneath Blackrock Mountain. Moira Bronzebeard married their emperor,"
        " and her son is heir to both lines." },
    { "furbolg", 0, "furbolg,furbolgs,timbermaw",
        "The furbolgs are peaceful bear people of Kalimdor's forests who honoured the demigods of the wild. "
        "The demonic corruption of Felwood and Ashenvale drove most tribes mad; the Timbermaw are the last "
        "unspoiled tribe and guard their tunnels against outsiders. They are slow to trust and loyal to "
        "friends." },
    { "undead gnome", 0, "undead gnome,forsaken gnome",
        "Some gnomes who died in the war or in Gnomeregan's radiation rose again. Neither the gnomes nor the "
        "Forsaken quite know what to do with them. They keep their clever minds and their love of tinkering, "
        "now with an unsettling patience." },
    { "ogre", 0, "ogre,ogres,gordunni,stonemaul,dire maul,gorian",
        "Ogres came from Draenor with the Horde, where they once ruled the Gorian Empire. Ogre magi were made"
        " by the orc warlocks. Clans like the Gordunni and the Stonemaul live in Feralas, Dustwallow and "
        "Desolace; some serve the Horde. They are strong, slow and fond of food and fighting." },
    { "two-headed ogre", 0, "two-headed ogre,ogre mage,ogre magi",
        "Two-headed ogres often became ogre magi, whose two heads argue with each other constantly. They are "
        "smarter than their one-headed kin and often lead clans. Cho'gall, the twilight cult's chieftain, was"
        " the most infamous of them." },
    { "dracthyr", 0, "dracthyr,neltharion,forbidden reach,evoker",
        "The dracthyr are dragonkin created by Neltharion, the black Aspect, before his madness made him "
        "Deathwing. They combine the powers of every dragonflight and can take dragon or humanoid shape. Long"
        " sealed away in the Forbidden Reach, they still search for their place in the world and resent what "
        "their maker did." },
    { "drakonid", 0, "drakonid,drakonids,blackwing lair,nefarian",
        "The drakonids are dragonkin bred by Nefarian in Blackwing Lair from the eggs and blood of the "
        "dragonflights. Strong, winged and fierce, they served the black dragonflight as soldiers. Freed from"
        " their masters, some fight for themselves, proud of their draconic strength." },
    { "drakkin", 0, "drakkin",
        "The drakkin are half-dragon, half-mortal, the children of dragons who took mortal shape and lived "
        "among the younger races. They carry a trace of their dragonflight in their blood and often in their "
        "eyes and horns. Each feels drawn to the duty of the flight their dragon parent belonged to." },
    { "eredar", 0, "eredar,argus,sargeras,kil'jaeden,archimonde",
        "The eredar of Argus were a brilliant, gifted people until Sargeras offered them power. Kil'jaeden "
        "and Archimonde accepted and became lords of the Burning Legion; only Velen and his followers fled. "
        "The eredar who remained became man'ari, demons. An eredar who does not serve the Legion is a rare "
        "thing indeed." },
    { "lightforged draenei", 0, "lightforged,lightforged draenei,army of the light,turalyon,xe'ra",
        "The Lightforged draenei have fought the Burning Legion on its own worlds for thousands of years with"
        " the Army of the Light. The naaru Xe'ra infused them with holy Light, which glows through their "
        "skin. They are zealous, disciplined and unbending, and they see the war against the Legion "
        "everywhere." },
    { "harpy", 0, "harpy,harpies,windfury,bloodfury",
        "Harpies are winged women descended, the stories say, from cursed night elves. They roost in the "
        "cliffs of Stonetalon, the Barrens and Thousand Needles, where clans like the Windfury and Bloodfury "
        "raid travellers. They are vain, cruel and quick to anger." },
    { "highmountain tauren", 0, "highmountain,highmountain tauren,thunder totem,huln",
        "The Highmountain tauren live on the peaks of Highmountain in a land of moose and eagles. Their hero "
        "Huln fought the Legion beside the night elves ten thousand years ago. The tribes - Riverhorn, "
        "Skyhorn, Bloodtotem and Rivermane - meet at Thunder Totem. They have antlers rather than horns and "
        "are proud mountain folk." },
    { "mechagnome", 0, "mechagnome,mechagnomes,mechagon,mekkatorque",
        "The mechagnomes are gnomes who replaced flesh with machinery on the island of Mechagon. Their mad "
        "king Mechagon wanted to purge all flesh; when he fell they turned to their gnome cousins. They are "
        "logical, precise and fascinated by improving everything, themselves included." },
    { "haranir", 0, "haranir",
        "The haranir are an ancient root people from the deep places of the world, who tend the roots of "
        "great trees and the hidden life underground. They seldom come to the surface and see the surface "
        "peoples as hurried and loud." },
    { "windshaper skyborne", 0, "skyborne,windshaper",
        "The skyborne windshapers are a winged people of the high peaks who read the winds and ride them. "
        "They live in aeries above the clouds and come down only when the winds bring them. They speak of the"
        " sky as others speak of their homeland." },
    { "high order skyborne", 0, "skyborne,high order",
        "The High Order is the ruling caste of the skyborne, keepers of their oldest rites and laws. Its "
        "members carry themselves with authority and regard other races with distant curiosity." },
    { "jinyu", 0, "jinyu,pandaria,waterspeaker",
        "The jinyu are a fish people of Pandaria's rivers who once served the mogu and won their freedom with"
        " the pandaren. Their waterspeakers read the future in the currents. They are proud, honourable and "
        "in old rivalry with the hozen." },
    { "arakkoa", 0, "arakkoa,sethekk,terokk,skettis,anzu",
        "The arakkoa are bird people of Draenor whose capital of Skettis hides in the forests of Terokkar. "
        "The Sethekk follow the raven god Anzu and his madness; the outcasts were cursed into flightlessness "
        "by their own priests. They are clever, secretive and deeply superstitious." },
    { "sethrak", 0, "sethrak,sethraliss,vol'dun",
        "The sethrak are snake people of Vol'dun's deserts. They worshipped the loa Sethraliss until her "
        "death, and many now serve dark masters in hope of reviving her. They are fearsome warriors and "
        "enemies of the vulpera and the Zandalari." },
    { "gnoll", 0, "gnoll,gnolls,riverpaw,mosshide,hogger",
        "Gnolls are hyena people who live in packs across the Eastern Kingdoms and Kalimdor - the Riverpaw of"
        " Westfall and Redridge, the Mosshide of the Wetlands. They are crude raiders who follow the "
        "strongest, Hogger being the most infamous. Few outsiders trust them, and gnolls trust no one." },
    { "saberon", 0, "saberon",
        "The saberon are cat people of Draenor who hunt in the jungles of Gorgrond and Tanaan. They are fast,"
        " ferocious hunters who follow strong leaders and love a good chase." },
    { "grummle", 0, "grummle,grummles,kun-lai",
        "The grummles are small, cheerful porters of Pandaria's mountains who carry goods across Kun-Lai "
        "Summit. They believe deeply in luck and carry lucky charms everywhere. Friendly, superstitious and "
        "always ready to trade." },
    { "kyrian", 0, "kyrian,bastion",
        "The kyrian are winged souls from Bastion, a realm of the Shadowlands where souls serve and protect "
        "the dead. They are devoted to duty and humility, and they find the chaos of mortal life strange and "
        "touching." },
    { "venthyr", 0, "venthyr,revendreth",
        "The venthyr are the aristocrats of Revendreth, a gothic realm where souls atone for pride. Elegant, "
        "cold and calculating, they collect sins and anima. They walk the mortal world with old courtesy and "
        "hidden purposes." },
    { "slime giant", 0, "slime,ooze",
        "The slime giants are great oozes that gained a mind of their own, usually from alchemists' accidents"
        " or the plague cauldrons of the Scourge. They understand the world only slowly but they have a "
        "surprisingly calm temper." },
    { "centaur", 0, "centaur,centaurs,kolkar,gelkis,magram,desolace,theradras",
        "The centaur are the children of Zaetar and the earth princess Theradras. Tribes like the Kolkar, "
        "Magram, Gelkis and Galak roam the Barrens and Desolace and have hunted the tauren for generations. "
        "They are proud, warlike nomads who respect only strength." },
    { "hozen", 0, "hozen,pandaria,monkey",
        "The hozen are monkey people of Pandaria who live for today: quick-tempered, crude, funny and always "
        "fighting. Their memory for grudges is short, their lifespan shorter, and they despise the jinyu. A "
        "hozen's word changes with the wind." },
    { "sylvari", 0, "sylvari,pale tree",
        "The sylvari are plant people grown from a great tree, born adult and connected to each other through"
        " a shared dream. Each has a calling from the dream that drives their life. They are curious about "
        "everything, but some are troubled by the darker voices in the dream." },
    { "mantid", 0, "mantid,klaxxi,empress,mantid empire",
        "The mantid are insects of Pandaria serving their empress in an ancient swarm cycle. The Klaxxi "
        "paragons keep their traditions; the mantid serve the Old God Y'Shaarj's legacy. They consider the "
        "soft races weak and short-lived." },
    { "ethereal", 0, "ethereal,ethereals,consortium,k'aresh,netherstorm",
        "The ethereals are beings of energy wrapped in cloth and bands, survivors of the world K'aresh, which"
        " the void lord Dimensius destroyed. The Consortium are traders and smugglers who sell everything "
        "across Outland; the Protectorate fights the Shadow Council. They speak in calm tones and always look"
        " for a profit." },
    { "banshee", 0, "banshee,banshees,sylvanas",
        "Banshees are the spirits of elven women whom the Scourge tore out of their bodies. Sylvanas "
        "Windrunner was the first; she freed many and gave them a home among the Forsaken. Their wail can "
        "drive the living mad, and their bitterness never fades." },
    { "faceless one", 0, "faceless,faceless one,old gods,c'thun,yogg-saron",
        "The faceless ones are servants of the Old Gods, born of the void and the corruption below the world."
        " They serve C'Thun, Yogg-Saron and N'Zoth in the dark places of Ahn'Qiraj, Ulduar and the deep. A "
        "faceless one outside their masters' service is a nightmare that walks." },
    { "elune's chosen", 0, "elune's chosen,elune,moon",
        "Elune's Chosen are night elves the moon goddess marked as her own: priestesses, sentinels and seers "
        "whose eyes shine with silver light. They believe Elune guides every step, and they bring her light "
        "into the darkest corners." },
    { "qiraji", 0, "qiraji,ahn'qiraj,silithus,c'thun,silithid",
        "The qiraji are insects of the Old God C'Thun, ruling the silithid swarms from Ahn'Qiraj in Silithus."
        " A thousand years ago they were sealed behind the Scarab Wall by night elves and bronze dragons. "
        "They are cruel, cunning and hungry for the surface world." },
    { "dreadlord", 0, "dreadlord,dreadlords,nathrezim,mal'ganis,tichondrius,varimathras",
        "The nathrezim, the dreadlords, are demons of the Burning Legion who rule through lies and "
        "manipulation. Mal'Ganis led Arthas to his fall; Tichondrius and Varimathras served the Lich King and"
        " Sylvanas. They are elegant, clever and utterly untrustworthy." },
    { "titan", 0, "titan,titans,pantheon,aman'thul,keepers",
        "The titans are world-forging giants of the cosmos who shaped Azeroth, bound the Old Gods and placed "
        "keepers in Ulduar. Their creations - earthen, mechagnomes, giants, vrykul - inherit their marks. A "
        "being of titan origin sees the world as a work in progress." },
    { "stormforged", 0, "stormforged,storm peaks,ulduar,loken",
        "The stormforged are iron and lightning beings the titans built in the Storm Peaks. Under Loken's "
        "corruption, many serve Yogg-Saron in Ulduar; others still guard the titan sites. They speak of duty,"
        " order and the keepers." },
    { "sporeling", 0, "sporeling,sporelings,sporeggar,zangarmarsh",
        "The sporelings are small mushroom people of Zangarmarsh in Outland. Their village of Sporeggar "
        "trades with outsiders who help them against the naga that drain the marsh. They are timid, friendly "
        "and know every mushroom of the bog." },
    { "fungarian", 0, "fungarian,fungus",
        "The fungarians are giant fungus beings of Zangarmarsh's depths, the elders of the sporelings. They "
        "grow slowly and think slowly, but they remember the marsh's whole history." },
    { "lich", 0, "lich,liches,kel'thuzad,cult of the damned",
        "Liches are sorcerers who gave up their bodies to live forever in undeath, binding their souls in "
        "phylacteries. Kel'Thuzad, founder of the Cult of the Damned, is the most famous lich and rules "
        "Naxxramas. Liches command the Scourge's armies with cold, endless intelligence." },
    { "nerubian", 0, "nerubian,nerubians,azjol-nerub,anub'arak,ahn'kahet",
        "The nerubians are spider people who built the vast underground kingdom of Azjol-Nerub beneath "
        "Northrend. The Lich King destroyed it in the War of the Spider and raised their king Anub'arak as "
        "his servant. Free nerubians still fight the Scourge from hidden halls." },
    { "sureki", 0, "sureki,nerubian,azj-kahet",
        "The sureki are a nerubian people of the deep kingdoms, ruled by an ambitious aristocracy. Their webs"
        " and cities hide far below the world, and their politics are as tangled as their webs." },
    { "mo'arg", 0, "mo'arg,felguard,gan'arg,doomguard",
        "The mo'arg are demon engineers and smiths of the Burning Legion, makers of its war machines and fel "
        "weapons. The gan'arg tinkers and the felguards are their kin. Ugly, strong and inventive, they build"
        " destruction with pride." },
    { "mogu", 0, "mogu,pandaria,thunder king,lei shen",
        "The mogu are stone giants of Pandaria who ruled the land as tyrants for millennia. The Thunder King "
        "Lei Shen enslaved the pandaren, jinyu and hozen until the pandaren revolution overthrew them. They "
        "believe they are destined to rule." },
};
const size_t RaceCount = sizeof(Races) / sizeof(Races[0]);

const LoreEntry Classes[] = {
    { "", 1, "warrior,warriors",
        "Warriors are trained soldiers of every people - the knights and footmen of the human kingdoms, the "
        "orc grunts, the tauren braves. They learn weapon and shield from masters in every city and guard the"
        " front line. A warrior's worth is shown in battle, not in words." },
    { "", 2, "paladin,paladins,silver hand,argent crusade,blood knight",
        "Paladins are holy warriors of the Light. Archbishop Alonsus Faol and Uther the Lightbringer founded "
        "the Knights of the Silver Hand in the Second War; Arthas betrayed and destroyed it. The Argent "
        "Crusade now carries its banner against the Scourge, the blood knights of Silvermoon learned the "
        "Light anew, and the draenei Vindicators have served it for ages." },
    { "", 3, "hunter,hunters,beast,pet",
        "Hunters live with the wild and fight beside a beast companion they tamed: wolves, cats, raptors, "
        "bears. Dwarven riflemen, night elf sentinels, troll headhunters and tauren hunters all hunt this "
        "way. They read tracks, set traps and shoot from far away." },
    { "", 4, "rogue,rogues,ravenholdt,SI:7,defias",
        "Rogues are thieves, spies and assassins. SI:7 serves Stormwind's king in secret, the Shattered Hand "
        "in Orgrimmar, the house of Ravenholdt serves whoever pays it. They fight with daggers, poison and "
        "shadows and trust only their own skill." },
    { "", 5, "priest,priests,holy light,shadow",
        "Priests serve faith - the Holy Light of the human Church, the Elune of the night elves, the Light of"
        " the naaru - and some walk the path of shadow. They heal the wounded, comfort the dying and drive "
        "out darkness, or turn darkness against their enemies." },
    { "", 6, "death knight,death knights,ebon blade,acherus,ebon hold",
        "Death knights are fallen heroes the Lich King raised as his champions. At the battle of Light's Hope"
        " Chapel many broke free under Highlord Darion Mograine and founded the Knights of the Ebon Blade, "
        "which now fights the Lich King from Acherus. The living fear them, and they bear the cold of the "
        "grave." },
    { "", 7, "shaman,shamans,elements,earthen ring,spirits",
        "Shamans speak with the elements - earth, fire, water and air - and with the spirits of the "
        "ancestors. Thrall restored shamanism among the orcs after the warlocks' corruption; the tauren, "
        "trolls and draenei have their own traditions. The Earthen Ring keeps the elements in balance." },
    { "", 8, "mage,mages,kirin tor,dalaran,arcane",
        "Mages study the arcane with discipline and books. The Kirin Tor of Dalaran is the greatest order of "
        "magi, now floating above Northrend to fight Malygos and the Lich King; Silvermoon's magisters and "
        "the gnomish arcanists have their own schools. Arcane magic is powerful and dangerous, and the Legion"
        " loves to lure those who use it." },
    { "", 9, "warlock,warlocks,demon,shadow council,fel",
        "Warlocks command demons and fel fire. Gul'dan's Shadow Council corrupted the orcs; human, gnome and "
        "Forsaken warlocks study in secret. They believe they control their demons; most people believe it is"
        " only a matter of time before the demons control them." },
    { "", 10, "hero,heroes",
        "Heroes refuse to be bound by a single discipline. They learn from every teacher who will have them -"
        " the sword of the warrior, the spells of the mage, the faith of the priest - and shape their own way"
        " of fighting. The orders look at them with suspicion, but many legends of Azeroth began exactly like"
        " that." },
    { "", 11, "druid,druids,cenarion circle,moonglade,emerald dream",
        "Druids follow the ways of nature, taught by the demigod Cenarius. Malfurion Stormrage was the first "
        "druid of the night elves; the Cenarion Circle gathers druids of all races in Moonglade. They take "
        "the shapes of bear, cat and bird, heal with nature and walk in the Emerald Dream." },
    { "", 12, "barbarian,barbarians,rage",
        "Barbarians fight with raw rage and strength instead of discipline. They come from the wilder peoples"
        " - the clans of the north, the hill tribes, the frontier - and train by surviving. Civilised "
        "soldiers think them savage; barbarians think civilised soldiers soft." },
    { "", 13, "witch doctor,witch doctors,voodoo,hex,loa",
        "Witch doctors practise voodoo: hexes, potions, curses and the calling of spirits and loa. The troll "
        "tradition is the oldest, but others have learned it too. They are healers and poisoners at once, "
        "respected and feared in every village." },
    { "", 14, "felsworn,demon hunter,illidari,fel",
        "The Felsworn wield the fel itself to hunt demons, in the tradition of Illidan's demon hunters. They "
        "took the Legion's power into their own bodies so they could fight it. Their own people distrust "
        "them, and many end up as the very thing they hunt." },
    { "", 15, "witch hunter,witch hunters,curse,cult",
        "Witch hunters track down curses, cultists, necromancers and those who make pacts with darkness. The "
        "orders of Gilneas and Kul Tiras trained the best of them. They are hard, suspicious people who carry"
        " silver, fire and an unshakable conviction." },
    { "", 16, "stormbringer,stormbringers,storm,lightning,wind",
        "Stormbringers call storms: lightning, wind and thunder obey them. Some learned it from the shamans, "
        "others from the storm itself on the peaks of the north. Their temper is said to change with the "
        "weather." },
    { "", 17, "knight of xoroth,xoroth,dreadsteed,fleshwarden",
        "The Knights of Xoroth are warrior-knights who bound themselves to the burning realm of Xoroth, home "
        "of the dreadsteeds. Fel fire and terror are their weapons, and their mounts are demons. They are "
        "feared on every battlefield and trusted on few." },
    { "", 18, "guardian,guardians,shield,protector",
        "Guardians are protectors above all: shield-bearers who place themselves between danger and those who"
        " cannot defend themselves. Every people has its guardians - the shieldmaidens of the north, the city"
        " watches, the temple guards. Their pride is in who survived because of them." },
    { "", 19, "templar,templars,holy order",
        "Templars are warriors of a holy order who combine discipline of body with faith. They train in "
        "monasteries and temples, fight with fists and blades and meditate between battles. Their vows bind "
        "them to protect the faithful and fight darkness." },
    { "", 20, "bloodmage,blood mage,blood magic",
        "Bloodmages draw magic from blood and life itself - their own and others'. The art came from Arugal's"
        " dark experiments and the old sorcerers of the Scourge war. Respectable mages call it forbidden; "
        "bloodmages call it honest, because every power has a price." },
    { "", 21, "ranger,rangers,farstriders,bow",
        "Rangers are archers and scouts of the wilds. The Farstriders of Quel'Thalas are the most famous "
        "order, and Sylvanas was their ranger-general before her death. They move silently, shoot from afar "
        "and know every path in the forest." },
    { "", 22, "chronomancer,chronomancers,time,bronze dragonflight,caverns of time,nozdormu",
        "Chronomancers bend time, a magic the bronze dragonflight of Nozdormu guards jealously in the Caverns"
        " of Time in Tanaris. They can slow, hasten and turn back moments. The bronze dragons watch them "
        "closely, because time is fragile and the Infinite Dragonflight wants to break it." },
    { "", 23, "necromancer,necromancers,cult of the damned,undead,death",
        "Necromancers raise and command the dead. Kel'Thuzad's Cult of the Damned spread the art across "
        "Lordaeron and made the Scourge possible; Scholomance was its school. A necromancer who does not "
        "serve the Lich King still walks a road most people find unforgivable." },
    { "", 24, "pyromancer,pyromancers,fire",
        "Pyromancers devote themselves to fire alone - the fiercest and most destructive of the elements. "
        "Many learned from mages, some from Ragnaros' cults or the fire elementals themselves. They love what"
        " burns and are often as hot-tempered as their spells." },
    { "", 25, "cultist,cultists,old gods,void,twilight's hammer",
        "Cultists serve powers beyond the world: the Old Gods, the void, the whispers in the deep. The "
        "Twilight's Hammer and other cults spread this faith in secret. Most people see them as madmen; "
        "cultists see the rest of the world as blind." },
    { "", 26, "starcaller,starcallers,stars,moon,cosmos",
        "Starcallers draw power from the stars and moons and read the signs in the night sky. The night elf "
        "priestesses of Elune and the astrologers of ancient peoples taught them. They speak of fate written "
        "above and of the cosmos that dwarfs every mortal quarrel." },
    { "", 27, "sun cleric,sun clerics,sun,an'she,sunwalker",
        "Sun clerics serve the sun: An'she of the tauren, the Sunwell of the elves, the warmth that brings "
        "life. They heal, banish shadows and burn the undead with sunlight. They are warm-hearted and "
        "stubborn against despair." },
    { "", 28, "tinker,tinkers,engineering,gadget,gnomish,goblin",
        "Tinkers fight with inventions - turrets, bombs, mechanical companions and armour of their own "
        "design. Gnomes and goblins are the masters, with very different ideas of safety. A tinker's best "
        "weapon is the one they haven't built yet." },
    { "", 29, "venomancer,venomancers,poison,serpent,venom,prophet",
        "Venomancers master poisons, serpents and slow death. They learned from the snake cults of the "
        "trolls, the naga and the desert peoples. Patient and cunning, they believe that the strongest enemy "
        "falls to the smallest bite." },
    { "", 30, "reaper,reapers,souls,scythe",
        "Reapers harvest souls with scythe and shadow. They stand at the border between life and death and "
        "know the moment a soul leaves its body. Others fear them as heralds of death, and reapers do not "
        "argue." },
    { "", 31, "primalist,primalists,wild,primal,beasts",
        "Primalists call on the raw, untamed wild - the fury of beasts, storms and growth. Unlike druids they"
        " seek no balance, only the primal power itself. They are at home far from cities, among beasts and "
        "storms." },
    { "", 32, "runemaster,runemasters,runes,spirit mage",
        "Runemasters carve power into symbols: runes on stone, weapons and skin. The art goes back to the "
        "titans and the vrykul and dwarves who inherited their marks. A runemaster sees magic in every "
        "carving and fears what a careless rune can do." },
};
const size_t ClassCount = sizeof(Classes) / sizeof(Classes[0]);

const LoreEntry Zones[] = {
    { "Durotar", 14, "durotar,razor hill,valley of trials,sen'jin village",
        "Durotar is a red, dry land on Kalimdor's east coast that Thrall named after his father. Orgrimmar "
        "stands in its north, Razor Hill and Sen'jin Village along the coast; centaur, quilboars and the "
        "Burning Blade cult trouble its outskirts." },
    { "Orgrimmar", 1637, "orgrimmar,grommash hold,valley of honor,the drag",
        "Orgrimmar, the capital of the Horde, is a fortress city of wood, iron and stone in a canyon of "
        "northern Durotar. Thrall rules from Grommash Hold; the Drag, the Valley of Honor and the Cleft of "
        "Shadow are always loud with trade and drill." },
    { "Mulgore", 215, "mulgore,bloodhoof village,red cloud mesa,camp narache",
        "Mulgore is a land of green grassy plains and mesas, home of the tauren. Bloodhoof Village lies at "
        "its heart; quilboars, harpies, gnolls and the Venture Company's loggers and miners threaten its "
        "peace." },
    { "Thunder Bluff", 1638, "thunder bluff,spirit rise,elder rise,hunter rise",
        "Thunder Bluff, the tauren capital, stands on tall mesas in northern Mulgore, linked by rope bridges "
        "and reached by elevators. Cairne Bloodhoof rules from the central rise; the Spirit Rise holds the "
        "spirit healers and the Pools of Vision." },
    { "The Barrens", 17, "barrens,crossroads,ratchet,camp taurajo,wailing caverns",
        "The Barrens is a vast dry savannah in central Kalimdor. The Crossroads is the Horde's hub, Ratchet a"
        " goblin port; centaur, quilboars, harpies and raptors roam it, and the Wailing Caverns hold the "
        "druids of the Fang." },
    { "Alterac Mountains", 36, "alterac,alterac mountains,strahnbrad,ravenholdt",
        "The Alterac Mountains are cold peaks where the kingdom of Alterac fell after its lord Perenolde "
        "betrayed the Alliance to the Horde. Ogres, Syndicate thieves and the Ravenholdt assassins hide among"
        " its ruins." },
    { "Arathi Highlands", 45, "arathi,arathi highlands,stromgarde,refuge pointe,hammerfall,arathi basin",
        "The Arathi Highlands hold the ruins of Stromgarde and the oldest human kingdom, Arathor. The "
        "Alliance holds Refuge Pointe, the Horde holds Hammerfall; Syndicate, ogres and the Forsaken fight "
        "over the rest." },
    { "Badlands", 3, "badlands,uldaman,kargath",
        "The Badlands are a scorched, dusty wasteland of canyons east of Khaz Modan. Kargath is a Horde "
        "outpost; the Dark Iron dwarves, ogres and black dragonkin roam it, and Uldaman hides titan secrets." },
    { "Blasted Lands", 4, "blasted lands,dark portal,nethergarde,dreadmaul",
        "The Blasted Lands are a red, dead wasteland, scarred where the Dark Portal opened for the orcish "
        "Horde. Nethergarde Keep watches the Portal; demons and ogres still roam the land, and the way to "
        "Outland lies through it." },
    { "Tirisfal Glades", 85, "tirisfal,tirisfal glades,brill,deathknell,scarlet monastery",
        "Tirisfal Glades is a gloomy, eternally autumn land around the ruins of Lordaeron's capital. Brill is"
        " the Forsaken village; the Scarlet Crusade still holds its monastery in the northeast." },
    { "Undercity", 1497, "undercity,royal quarter,apothecarium",
        "The Undercity was built in the catacombs beneath the ruined capital of Lordaeron. Sylvanas rules the"
        " Forsaken from the Royal Quarter; the Apothecarium works on its plague, and sewage flows through its"
        " canals." },
    { "Silverpine Forest", 130, "silverpine,silverpine forest,the sepulcher,shadowfang keep,pyrewood",
        "Silverpine Forest is a dark coastal forest between Tirisfal and Gilneas. The Forsaken hold the "
        "Sepulcher; worgen haunt Shadowfang Keep, where Archmage Arugal brought them, and the villagers of "
        "Pyrewood turn into beasts at night." },
    { "Western Plaguelands", 28, "western plaguelands,andorhal,chillwind camp,scholomance,hearthglen",
        "The Western Plaguelands were the farmland of Lordaeron, ravaged by the plague. Andorhal lies in "
        "ruins, the Scholomance teaches necromancy on Caer Darrow, and the Scarlet Crusade, the Argent Dawn "
        "and the Scourge fight over the fields." },
    { "Eastern Plaguelands", 139, "eastern plaguelands,light's hope,stratholme,naxxramas,tyr's hand",
        "The Eastern Plaguelands are the worst-plagued land of Lordaeron. Light's Hope Chapel stands for the "
        "Argent Dawn, burned Stratholme holds the Scourge and the Scarlet Crusade, and Naxxramas once hovered"
        " above it." },
    { "Hillsbrad Foothills", 267, "hillsbrad,southshore,tarren mill,durnholde",
        "Hillsbrad Foothills are green hills where Southshore of the Alliance and Tarren Mill of the Forsaken"
        " face each other. Thrall was once a slave in Durnholde Keep there; the Syndicate and ogres lurk in "
        "the hills." },
    { "The Hinterlands", 47, "hinterlands,aerie peak,revantusk,jintha'alor",
        "The Hinterlands are wild green highlands, home of the Wildhammer dwarves of Aerie Peak and their "
        "gryphons. The Revantusk trolls side with the Horde; the Witherbark and the trolls of Jintha'Alor "
        "hold old Amani sites." },
    { "Dun Morogh", 1, "dun morogh,kharanos,coldridge,gnomeregan",
        "Dun Morogh is a snowy mountain land around Ironforge. Kharanos and its brewery are the heart of the "
        "dwarves; troggs, ice trolls and leper gnomes from Gnomeregan trouble the valleys." },
    { "Ironforge", 1537, "ironforge,great forge,tinker town,hall of explorers",
        "Ironforge is a city carved into a mountain around the Great Forge. King Magni rules from the High "
        "Seat; the gnomes of Tinker Town and the Hall of Explorers live here, and the Deeprun Tram connects "
        "it to Stormwind." },
    { "Searing Gorge", 51, "searing gorge,thorium point,cauldron",
        "The Searing Gorge is a burning wasteland created when Thaurissan summoned Ragnaros. The Dark Iron "
        "dwarves work the Cauldron with slaves; the Thorium Brotherhood holds Thorium Point and trades with "
        "all who prove themselves." },
    { "Burning Steppes", 46, "burning steppes,blackrock mountain,blackrock,morgan's vigil",
        "The Burning Steppes were green land until Ragnaros was summoned; they have smouldered ever since. "
        "Blackrock Mountain rises over them, with the Dark Irons below and the black dragons and Blackrock "
        "orcs above." },
    { "Elwynn Forest", 12, "elwynn,elwynn forest,goldshire,northshire,lion's pride",
        "Elwynn Forest is the green heart of the kingdom of Stormwind. Goldshire and its Lion's Pride Inn are"
        " the crossroads of travellers; Defias bandits, kobolds and gnolls make the woods less peaceful than "
        "they look." },
    { "Stormwind City", 1519, "stormwind,stormwind city,cathedral of light,trade district,old town,keep",
        "Stormwind, the capital of the human kingdom, was rebuilt in white stone after the orcs razed it. "
        "King Varian Wrynn rules from Stormwind Keep; the Cathedral of Light, the Trade District, Old Town "
        "and the canals fill its walls." },
    { "Deadwind Pass", 41, "deadwind pass,karazhan,medivh",
        "Deadwind Pass is a dead, grey valley around Karazhan, the tower of the mage Medivh, who opened the "
        "Dark Portal. The tower is haunted by ghosts and demons, and travellers avoid the pass." },
    { "Duskwood", 10, "duskwood,darkshire,raven hill,night watch",
        "Duskwood is a forest of eternal night, once part of Elwynn. Darkshire's Night Watch fights worgen, "
        "the undead of Raven Hill and the dark things the forest has bred since the Scourge and Medivh's "
        "shadow." },
    { "Loch Modan", 38, "loch modan,thelsamar,stonewrought dam",
        "Loch Modan is a lake valley behind the great Stonewrought Dam of the dwarves. Thelsamar is its "
        "village; troggs, kobolds and the Dark Iron dwarves raid it, and the Explorers' League digs there." },
    { "Redridge Mountains", 44, "redridge,lakeshire,stonewatch,blackrock orcs",
        "Redridge Mountains, with the town of Lakeshire on its lake, is the front line against the Blackrock "
        "orcs and gnolls who come down from the east. Stormwind's soldiers rarely have enough men there." },
    { "Stranglethorn Vale", 33, "stranglethorn,booty bay,grom'gol,zul'gurub,gurubashi",
        "Stranglethorn Vale is a jungle where the Gurubashi trolls once ruled from Zul'Gurub. Booty Bay is a "
        "neutral goblin port of pirates; hunters, raptors, panthers and the Bloodsail Buccaneers are "
        "everywhere." },
    { "Swamp of Sorrows", 8, "swamp of sorrows,stonard,sunken temple,atal'hakkar",
        "The Swamp of Sorrows is a murky swamp where the Horde holds Stonard. The Temple of Atal'Hakkar sank "
        "in its lake when the trolls tried to summon the blood god Hakkar; green dragons guard it." },
    { "Westfall", 40, "westfall,sentinel hill,defias,moonbrook,deadmines",
        "Westfall is a dry farmland whose farmers suffer from bandits of the Defias Brotherhood, who hide in "
        "the Deadmines under Moonbrook. The People's Militia holds Sentinel Hill; many farms lie abandoned." },
    { "Wetlands", 11, "wetlands,menethil harbor,grim batol,dragonmaw",
        "The Wetlands are a misty marsh with Menethil Harbor, the port to Northrend and Kalimdor. The "
        "Dragonmaw orcs once held red dragons captive at Grim Batol, and raptors, crocolisks and murlocs roam"
        " the bogs." },
    { "Teldrassil", 141, "teldrassil,dolanaar,shadowglen,rut'theran",
        "Teldrassil is the giant world tree the night elves planted off Kalimdor's north coast. Darnassus "
        "lies in its crown, Dolanaar in its forests; corruption spreads in its roots and furbolgs have gone "
        "mad." },
    { "Darnassus", 1657, "darnassus,temple of the moon,cenarion enclave",
        "Darnassus, the night elf capital in the crown of Teldrassil, is a city of temples and gardens. "
        "Tyrande Whisperwind leads the Sisterhood of Elune from the Temple of the Moon; the druids gather in "
        "the Cenarion Enclave." },
    { "Darkshore", 148, "darkshore,auberdine,blackwood",
        "Darkshore is a gloomy forested coast with the night elf town of Auberdine. Ancient elven ruins, "
        "corrupted furbolgs and the twilight cult haunt its shore." },
    { "Ashenvale", 331, "ashenvale,astranaar,warsong lumber camp,splintertree",
        "Ashenvale is an ancient forest of the night elves, where the orcs of the Warsong clan cut wood - the"
        " old war between them still burns here. Astranaar holds the elves, Splintertree the Horde; demons "
        "haunt the east." },
    { "Thousand Needles", 400, "thousand needles,freewind post,shimmering flats,mirage raceway",
        "Thousand Needles is a canyon of tall stone pillars. The tauren of Freewind Post live on top of them;"
        " below, centaur raid and the goblins race on the salt flats." },
    { "Stonetalon Mountains", 406, "stonetalon,sun rock retreat,stonetalon peak,venture company",
        "The Stonetalon Mountains are high, rugged peaks where the Venture Company tears down forests and the"
        " Horde and night elves both have outposts. Harpies roost in the cliffs." },
    { "Desolace", 405, "desolace,shadowprey,nijel's point,maraudon",
        "Desolace is a grey, barren land where the centaur tribes are constantly at war with each other. "
        "Maraudon, the burial place of Zaetar, lies under it; the Burning Legion's remnants and the Kolkar "
        "haunt the plains." },
    { "Feralas", 357, "feralas,feathermoon,camp mojache,dire maul",
        "Feralas is a wet, green forest of ancient trees. The ruins of Eldre'Thalas - Dire Maul - house ogres"
        " and Highborne ghosts; the night elves of Feathermoon and the tauren of Camp Mojache keep watch." },
    { "Dustwallow Marsh", 15, "dustwallow,theramore,brackenwall,onyxia",
        "Dustwallow Marsh is a swamp on Kalimdor's east coast. Jaina Proudmoore rules the human city of "
        "Theramore on its island; the black dragon Onyxia lairs in the south, and the Horde holds "
        "Brackenwall." },
    { "Tanaris", 440, "tanaris,gadgetzan,caverns of time,zul'farrak",
        "Tanaris is a hot desert with the goblin city of Gadgetzan. The Caverns of Time of the bronze "
        "dragonflight lie in its east; the trolls of Zul'Farrak and the Wastewander bandits make the dunes "
        "dangerous." },
    { "Azshara", 16, "azshara,eldarath,blue dragons",
        "Azshara is a land of eternal autumn and the ruins of the night elves' Highborne. Naga, satyrs and "
        "blue dragons haunt its coasts, and travellers rarely come." },
    { "Felwood", 361, "felwood,irontree,jadefire,timbermaw",
        "Felwood was once part of Ashenvale until the demons of the Burning Legion poisoned it. Its trees are"
        " rotten, its waters toxic, its creatures mad; the Timbermaw furbolgs guard their tunnels there." },
    { "Un'Goro Crater", 490, "un'goro,ungoro,marshal's refuge,devilsaur",
        "Un'Goro Crater is a hot, wild jungle in a crater full of dinosaurs, glowing crystals and silithid. "
        "The titans' old laboratory lies in it; Marshal's Refuge is the only camp." },
    { "Silithus", 1377, "silithus,cenarion hold,ahn'qiraj,scarab wall",
        "Silithus is a desert in Kalimdor's southwest, ruled by the silithid hives. The Cenarion Circle holds"
        " Cenarion Hold; behind the Scarab Wall the city of Ahn'Qiraj and the Old God C'Thun wait." },
    { "Winterspring", 618, "winterspring,everlook,mazthoril",
        "Winterspring is a snowy valley in Kalimdor's north. The goblin town of Everlook trades there; yetis,"
        " furbolgs, frostsabers and the blue dragons of Mazthoril roam the snow." },
    { "Moonglade", 493, "moonglade,nighthaven",
        "Moonglade is a peaceful valley sacred to the druids. Nighthaven is the home of the Cenarion Circle; "
        "no fighting is allowed there." },
    { "Eversong Woods", 3430, "eversong,eversong woods,falconwing,sunstrider isle",
        "Eversong Woods is the golden autumn forest of Quel'Thalas. The blood elves restored it after the "
        "Scourge's attack; magic and magisters are everywhere, and the Wretched lurk in the ruins." },
    { "Silvermoon City", 3487, "silvermoon,silvermoon city,sunfury spire,murder row",
        "Silvermoon is the shining capital of the blood elves, half of it rebuilt, half still in ruins after "
        "Arthas' march. Regent Lord Lor'themar Theron rules from the Sunfury Spire; magic keeps the city "
        "clean and running." },
    { "Ghostlands", 3433, "ghostlands,tranquillien,deatholme,zul'aman",
        "The Ghostlands are the dead southern part of Quel'Thalas, scorched by the Scourge's march. "
        "Tranquillien is the blood elves' outpost; Deatholme holds the Scourge, and Zul'Aman lies to the "
        "east." },
    { "Azuremyst Isle", 3524, "azuremyst,azure watch,ammen vale",
        "Azuremyst Isle is a blue island off Kalimdor's coast where the Exodar crashed. The draenei build "
        "there; crystals from the ship poisoned the land, and Sunhawk spies of the blood elves hide in the "
        "woods." },
    { "The Exodar", 3557, "exodar,the exodar,vault of lights",
        "The Exodar is the crashed naaru ship of the draenei, now their city. Prophet Velen leads them from "
        "the Vault of Lights, where the naaru O'ros hovers." },
    { "Bloodmyst Isle", 3525, "bloodmyst,blood watch",
        "Bloodmyst Isle, north of Azuremyst, was turned blood-red by the Exodar's crystals. The draenei fight"
        " the Sunhawk blood elves and the satyrs there." },
    { "Hellfire Peninsula", 3483, "hellfire,hellfire peninsula,honor hold,thrallmar,hellfire citadel",
        "Hellfire Peninsula is the red wasteland around the Dark Portal in Outland. Honor Hold of the "
        "Alliance and Thrallmar of the Horde face the fel orcs of Hellfire Citadel and the Burning Legion's "
        "war machines." },
    { "Zangarmarsh", 3521, "zangarmarsh,cenarion refuge,sporeggar,serpentshrine,telredor,zabra'jin",
        "Zangarmarsh is a marsh of giant glowing mushrooms in Outland. The naga of Lady Vashj drain its "
        "waters, the Cenarion Expedition defends it, and the sporelings of Sporeggar live in the bog." },
    { "Terokkar Forest", 3519, "terokkar,shattrath,auchindoun,allerian,stonebreaker",
        "Terokkar Forest is a forest in Outland with Shattrath City in its north. The ruins of Auchindoun, "
        "the bone wastes and the arakkoa of Skettis lie in its south." },
    { "Shattrath City", 3703, "shattrath,shattrath city,aldor,scryers,terrace of light,a'dal",
        "Shattrath City is the refuge city of Outland, protected by the naaru A'dal. The Aldor draenei and "
        "the Scryer blood elves share it in an uneasy peace; refugees of every race live there." },
    { "Nagrand", 3518, "nagrand,garadar,telaar,halaa,throne of the elements",
        "Nagrand is green, floating-island grassland in Outland, the most beautiful land left on Draenor. The"
        " Mag'har orcs of Garadar and the Kurenai of Telaar live there; ogres and the Throne of the Elements "
        "lie in its plains." },
    { "Blade's Edge Mountains", 3522, "blade's edge,blades edge,sylvanaar,thunderlord,gruul",
        "Blade's Edge Mountains are jagged spires in Outland, home of the ogres of Gruul the Dragonkiller. "
        "Black dragons, arakkoa and the Burning Legion all have footholds there." },
    { "Netherstorm", 3523, "netherstorm,area 52,tempest keep,manaforge",
        "Netherstorm is a land that floats in the Twisting Nether, broken to pieces. Kael'thas' blood elves "
        "drain its energy with manaforges and rule from Tempest Keep; the goblins of Area 52 and the "
        "ethereals trade there." },
    { "Shadowmoon Valley", 3520, "shadowmoon,shadowmoon valley,black temple,wildhammer stronghold,shadowmoon village",
        "Shadowmoon Valley is a dark, fel-scorched land where Gul'dan's warlocks first drank demon blood. "
        "Illidan rules from the Black Temple, and the Dragonmaw orcs and demons hold the rest." },
    { "Isle of Quel'Danas", 4080, "quel'danas,isle of quel'danas,sunwell,magisters' terrace,shattered sun",
        "The Isle of Quel'Danas holds the Sunwell. Kael'thas tried to summon Kil'jaeden through it; the "
        "Shattered Sun Offensive of Aldor and Scryers fought to stop him." },
    { "Borean Tundra", 3537, "borean tundra,valiance keep,warsong hold,coldarra,nexus",
        "The Borean Tundra is a cold plain on Northrend's west coast. Valiance Keep of the Alliance and "
        "Warsong Hold of the Horde were the first footholds against the Scourge; Malygos' blue dragons rule "
        "Coldarra and the Nexus." },
    { "Howling Fjord", 495, "howling fjord,valgarde,vengeance landing,utgarde,new agamand",
        "The Howling Fjord is a land of steep fjords in Northrend's southeast. Valgarde of the Alliance and "
        "Vengeance Landing of the Forsaken stand against the vrykul of Utgarde; the Forsaken's new plague is "
        "tested in New Agamand." },
    { "Dragonblight", 65, "dragonblight,wyrmrest,wrathgate,angrathar,naxxramas,azjol-nerub",
        "Dragonblight is the snowy graveyard of the dragons in central Northrend. Wyrmrest Temple of the "
        "dragonflights stands there; the Wrathgate at Angrathar leads to Icecrown, and Naxxramas looms in the"
        " east." },
    { "Grizzly Hills", 394, "grizzly hills,amberpine,conquest hold,drak'tharon",
        "The Grizzly Hills are a forested land of grizzly bears and furbolgs. Alliance and Horde fight over "
        "its lumber; the trolls of Drak'Tharon and the iron dwarves of the Storm Peaks press from both sides." },
    { "Zul'Drak", 66, "zul'drak,gundrak,argent stand,ebon hold",
        "Zul'Drak is the frozen empire of the Drakkari trolls, now overrun by the Scourge. The trolls "
        "sacrifice their own loa in desperation; the Argent Crusade and the Ebon Blade fight there." },
    { "Sholazar Basin", 3711, "sholazar,sholazar basin,nesingwary,oracles,wolvar",
        "Sholazar Basin is a hidden jungle in the middle of frozen Northrend, kept warm by titan machines. "
        "The wolvar and gorlocs fight there; Hemet Nesingwary's hunters have their camp." },
    { "Icecrown", 210, "icecrown,icecrown citadel,lich king,argent tournament,frozen throne",
        "Icecrown is the seat of the Lich King, a frozen wasteland full of the Scourge's armies. Icecrown "
        "Citadel rises over the Frozen Throne; the Argent Crusade holds its tournament grounds and the "
        "Alliance and Horde battle fleets hover above." },
    { "The Storm Peaks", 67, "storm peaks,ulduar,k3,dun niffelem,brunnhildar",
        "The Storm Peaks are the highest mountains of Northrend. Titan city Ulduar lies in them, where "
        "Yogg-Saron waits; the frost giants, storm giants and iron dwarves fight over the snow." },
    { "Crystalsong Forest", 2817, "crystalsong,crystalsong forest,dalaran,violet stand",
        "Crystalsong Forest is a forest of crystal trees, frozen by the magic of the blue dragonflight. "
        "Dalaran floats over it, and the Kirin Tor watch the war of the dragonflights from above." },
    { "Dalaran", 4395, "dalaran,violet citadel,krasus' landing,underbelly,silver covenant,sunreavers",
        "Dalaran is the floating city of the Kirin Tor above Northrend. Archmage Rhonin leads the magi "
        "against Malygos and the Lich King; Alliance and Horde both come there, with the Silver Covenant and "
        "the Sunreavers watching each other." },
    { "Wintergrasp", 4197, "wintergrasp,lake wintergrasp,wintergrasp fortress,vault of archavon",
        "Wintergrasp is a battlefield in Northrend where Alliance and Horde fight for the fortress and the "
        "titan Vault of Archavon beneath it. Whoever holds it gets its resources and its prestige." },
    { "Hrothgar's Landing", 4742, "hrothgar's landing,kvaldir",
        "Hrothgar's Landing is a rocky island in the north of Northrend where the Argent Crusade fights the "
        "Kvaldir sea raiders." },
};
const size_t ZoneCount = sizeof(Zones) / sizeof(Zones[0]);

const LoreEntry Topics[] = {
    { "lich king", 0, "lich king,arthas,frozen throne,ner'zhul,frostmourne",
        "The Lich King was once the orc shaman Ner'zhul, bound in the Frozen Throne by the Burning Legion. "
        "Prince Arthas Menethil of Lordaeron took up the cursed blade Frostmourne, killed his father, "
        "destroyed Quel'Thalas and merged with Ner'zhul on the Frozen Throne. Now he rules the Scourge from "
        "Icecrown and the world has brought its armies to Northrend." },
    { "scourge", 0, "scourge,undead army,plague,plague of undeath,ghoul,ghouls,abomination",
        "The Scourge is the Lich King's army of the dead. Kel'Thuzad's Cult of the Damned spread the plague "
        "through Lordaeron's grain; whoever ate it died and rose again. The Scourge destroyed Lordaeron and "
        "Quel'Thalas and now fights from Northrend, from Naxxramas and the Plaguelands." },
    { "burning legion", 0, "burning legion,legion,demons,sargeras,kil'jaeden,archimonde,eredar",
        "The Burning Legion is the endless demon army of Sargeras, the fallen titan, who wants to destroy all"
        " creation. Kil'jaeden and Archimonde lead it; it invaded Azeroth in the War of the Ancients and the "
        "Third War, where Archimonde fell at Mount Hyjal. Its agents still work in Outland and Azeroth." },
    { "horde", 0, "horde,for the horde,lok'tar",
        "The Horde under Warchief Thrall unites orcs, Darkspear trolls, tauren, Forsaken and blood elves. It "
        "was born of the old demonic Horde but now seeks honour and a home in Kalimdor. Many in the Alliance "
        "do not trust it, and Garrosh Hellscream pushes it toward a harder line in Northrend." },
    { "alliance", 0, "alliance,for the alliance",
        "The Alliance unites humans, dwarves, night elves, gnomes and draenei, led by King Varian Wrynn of "
        "Stormwind. It has fought the Horde for three wars and does not trust its old enemy, but now both "
        "fight the Lich King in Northrend." },
    { "thrall", 0, "thrall,go'el,warchief",
        "Thrall, son of Durotan, grew up a slave in Durnholde Keep, escaped, freed the orcs from the "
        "internment camps and became Warchief of the new Horde. He is a shaman who brought his people to "
        "Kalimdor and founded Orgrimmar." },
    { "varian wrynn", 0, "varian,varian wrynn,king of stormwind",
        "Varian Wrynn is the King of Stormwind, a famous gladiator who was missing for years after being "
        "kidnapped. He hates the Horde, especially after the Wrathgate, and leads the Alliance with a heavy "
        "hand." },
    { "jaina proudmoore", 0, "jaina,proudmoore,theramore",
        "Jaina Proudmoore is the mage and lady of Theramore, once a friend of Arthas. She led the human "
        "refugees to Kalimdor and allied with Thrall against Archimonde. She works for peace between Alliance"
        " and Horde." },
    { "sylvanas", 0, "sylvanas,windrunner,banshee queen,dark lady",
        "Sylvanas Windrunner was Ranger-General of Quel'Thalas until Arthas killed her and made her a "
        "banshee. Free of the Lich King, she now rules the Forsaken as the Dark Lady and wants revenge on "
        "Arthas above all else." },
    { "illidan", 0, "illidan,betrayer,stormrage,black temple",
        "Illidan Stormrage, twin brother of Malfurion, was imprisoned for ten thousand years for betraying "
        "his people with demonic power. Released, he devoured the Skull of Gul'dan and became half demon, "
        "then fled to Outland and rules from the Black Temple." },
    { "kael'thas", 0, "kael'thas,kael,sunstrider,prince kael",
        "Prince Kael'thas Sunstrider led the blood elves after the fall of Quel'Thalas. He sided with Illidan"
        " in Outland and drained the energy of Netherstorm from Tempest Keep; later he betrayed his people "
        "for Kil'jaeden." },
    { "cairne", 0, "cairne,bloodhoof",
        "Cairne Bloodhoof is the old High Chieftain of the tauren, who led them to Mulgore and allied with "
        "Thrall. He is wise, honourable and the voice of reason in the Horde." },
    { "vol'jin", 0, "vol'jin,darkspear",
        "Vol'jin is the chieftain of the Darkspear trolls, son of Sen'jin, who died in their escape. He is a "
        "shadow hunter loyal to Thrall and distrusts Garrosh." },
    { "garrosh", 0, "garrosh,hellscream",
        "Garrosh Hellscream, son of Grom Hellscream, commands the Horde's forces at Warsong Hold in "
        "Northrend. He is fierce, proud, impatient and eager to prove the Horde's might." },
    { "magni", 0, "magni,bronzebeard,king of ironforge",
        "Magni Bronzebeard is the King of Ironforge, brother of Brann and Muradin. He is a gruff, "
        "warm-hearted dwarf who studies the ancient tablets of Uldaman to find the origin of his people." },
    { "tyrande", 0, "tyrande,whisperwind,high priestess",
        "Tyrande Whisperwind is the High Priestess of Elune and leader of the night elves from Darnassus, "
        "beloved of Malfurion Stormrage. She fought in the War of the Ancients ten thousand years ago." },
    { "malfurion", 0, "malfurion,stormrage,archdruid",
        "Malfurion Stormrage is the first night elf druid, student of Cenarius, who fought the Legion in the "
        "War of the Ancients. He is lost in the Emerald Dream, which is being corrupted by the Nightmare." },
    { "velen", 0, "velen,prophet velen",
        "Prophet Velen led the draenei away from Argus when Sargeras seduced the eredar. He sees visions of "
        "the future and leads his people from the Exodar." },
    { "lor'themar", 0, "lor'themar,theron,regent",
        "Lor'themar Theron is the Regent Lord of Quel'Thalas who rules Silvermoon while Kael'thas was away. "
        "He is a practical leader who works to restore his people." },
    { "kel'thuzad", 0, "kel'thuzad,naxxramas",
        "Kel'Thuzad was an archmage of Dalaran who founded the Cult of the Damned and spread the plague. "
        "Arthas killed him and later raised him as a lich; he rules the floating necropolis of Naxxramas." },
    { "dragonflights", 0, "dragon,dragons,dragonflight,dragonflights,aspect,aspects,wyrmrest",
        "The five dragonflights were given their powers by the titans: the red of Alexstrasza guards life, "
        "the blue of Malygos magic, the green of Ysera the Emerald Dream, the bronze of Nozdormu time, and "
        "the black of Neltharion - now Deathwing - the earth, until he went mad. Malygos is waging war on all"
        " mortal mages in the Nexus War." },
    { "deathwing", 0, "deathwing,neltharion,onyxia,nefarian,black dragon,black dragons",
        "Deathwing, once Neltharion the Earth-Warder, was corrupted by the Old Gods and betrayed the "
        "dragonflights in the War of the Ancients. His children Onyxia and Nefarian lead the black "
        "dragonflight in Dustwallow and Blackrock Mountain." },
    { "old gods", 0, "old gods,old god,c'thun,yogg-saron,n'zoth,whispers,void",
        "The Old Gods are ancient evils the titans imprisoned beneath Azeroth: C'Thun in Ahn'Qiraj, "
        "Yogg-Saron in Ulduar and N'Zoth in the deep. Their whispers drive mortals mad, and their servants "
        "are the faceless ones and the twilight cults." },
    { "titans", 0, "titans,titan,keepers,ulduar,uldaman,uldum",
        "The titans are the world-shapers who ordered Azeroth, imprisoned the Old Gods and left keepers to "
        "guard it from Ulduar. Their creations include the earthen, the mechagnomes, the giants and the "
        "vrykul." },
    { "argent crusade", 0, "argent crusade,argent dawn,tirion,fordring,crusader",
        "The Argent Crusade, formed from the Argent Dawn and the Knights of the Silver Hand, is led by "
        "Highlord Tirion Fordring. It unites every race against the Scourge and holds the Argent Tournament "
        "in Icecrown." },
    { "ebon blade", 0, "ebon blade,knights of the ebon blade,acherus,darion mograine",
        "The Knights of the Ebon Blade are the death knights who broke free of the Lich King at Light's Hope "
        "Chapel under Darion Mograine. They now fight the Scourge from the floating necropolis of Acherus." },
    { "kirin tor", 0, "kirin tor,rhonin,violet citadel,mages of dalaran",
        "The Kirin Tor is the order of magi of Dalaran, led by Rhonin. It moved its city to Northrend to "
        "fight Malygos and the Lich King and opens its doors to both Alliance and Horde." },
    { "scarlet crusade", 0, "scarlet crusade,scarlet,crusaders,scarlet monastery,hearthglen,tyr's hand",
        "The Scarlet Crusade is a fanatical order that formed in Lordaeron to purge the plague. Its zealots "
        "kill anyone they suspect of undeath, Forsaken and innocents alike. They hold the Scarlet Monastery, "
        "Hearthglen and Tyr's Hand, and their Onslaught has come to Northrend." },
    { "cult of the damned", 0, "cult of the damned,cultists,necromancy,scholomance",
        "The Cult of the Damned is the human cult Kel'Thuzad founded to serve the Lich King. It spreads the "
        "plague, teaches necromancy at the Scholomance and recruits the desperate with promises of eternal "
        "life." },
    { "defias", 0, "defias,defias brotherhood,van cleef,stonemasons",
        "The Defias Brotherhood are the stonemasons who rebuilt Stormwind and were cheated of their pay by "
        "the nobles. Led by Edwin VanCleef, they turned to banditry in Westfall and hide in the Deadmines." },
    { "cenarion circle", 0, "cenarion circle,cenarion expedition,cenarius,druids",
        "The Cenarion Circle is the druid order of Moonglade, open to all races. It holds Cenarion Hold in "
        "Silithus against the silithid; its Expedition protects Zangarmarsh in Outland." },
    { "steamwheedle cartel", 0, "steamwheedle,cartel,booty bay,gadgetzan,ratchet,everlook",
        "The Steamwheedle Cartel is the great goblin trading cartel that runs the neutral ports of Booty Bay,"
        " Gadgetzan, Ratchet and Everlook. It trades with everyone and keeps its towns neutral with heavily "
        "armed bruisers." },
    { "naaru", 0, "naaru,a'dal,light,holy light",
        "The naaru are beings of pure Light who fight the Burning Legion. They guided the draenei; A'dal "
        "protects Shattrath. Many priests and paladins see the Light they embody as the source of their "
        "power." },
    { "elune", 0, "elune,moon goddess,mother moon",
        "Elune is the moon goddess of the night elves, the Mother Moon. Her priestesses, the Sisterhood of "
        "Elune, lead the night elves, and many believe she watches over all who walk in her light." },
    { "earthmother", 0, "earthmother,an'she,mu'sha,earth mother",
        "The Earthmother is the goddess of the tauren, mother of all. Her eyes are An'she, the sun, and "
        "Mu'sha, the moon. The tauren believe all life is sacred because it comes from her." },
    { "loa", 0, "loa,hakkar,shirvallah,bwonsamdi,hir'eek",
        "The loa are the spirits of the wild that the trolls worship and bargain with: the panther, the bat, "
        "the snake, the raptor. Hakkar the Soulflayer, the blood god, nearly destroyed the Gurubashi; the "
        "Drakkari now sacrifice their loa in desperation." },
    { "dark portal", 0, "dark portal,portal,outland,draenor",
        "The Dark Portal is the great gateway between Azeroth and Draenor in the Blasted Lands. The orcs came"
        " through it in the First War; it was reopened by the Burning Legion, and now it leads to Outland, "
        "the broken remains of Draenor." },
    { "wars", 0, "first war,second war,third war,war of the ancients,war,history",
        "In the War of the Ancients ten thousand years ago the night elves drove out the Burning Legion. In "
        "the First War the orcs razed Stormwind; in the Second War the Alliance defeated the Horde; in the "
        "Third War the Scourge destroyed Lordaeron, the Legion invaded Kalimdor and the races united to "
        "defeat Archimonde at Mount Hyjal." },
    { "wrathgate", 0, "wrathgate,angrathar,bolvar,putress,saurfang",
        "At the Wrathgate in Dragonblight, Alliance and Horde attacked the Lich King together. Grand "
        "Apothecary Putress of the Forsaken betrayed both and released a new plague; Bolvar Fordragon and "
        "Dranosh Saurfang died, and the Alliance blames the Horde." },
    { "northrend", 0, "northrend",
        "Northrend is the frozen continent of the north, home of the Lich King in Icecrown. Alliance and "
        "Horde have landed in the Borean Tundra and the Howling Fjord to bring the war to him; the vrykul, "
        "nerubians, tuskarr and dragons all fight their own battles there." },
    { "outland", 0, "outland",
        "Outland is all that remains of Draenor, the orc homeworld, torn apart by Ner'zhul's portals. "
        "Illidan, Kael'thas and the Burning Legion fight for it; Shattrath is the only city of refuge." },
    { "emerald dream", 0, "emerald dream,dream,nightmare,ysera",
        "The Emerald Dream is the green realm of Ysera, a vision of the world untouched by mortals. Druids "
        "walk there in their sleep; the Emerald Nightmare is now corrupting it, and Malfurion is lost inside." },
    { "ahn'qiraj", 0, "ahn'qiraj,silithid,qiraji,c'thun",
        "Ahn'Qiraj is the insect city of the Old God C'Thun in Silithus. A thousand years ago the night elves"
        " and the bronze dragons sealed it behind the Scarab Wall; when it opened again, every race fought "
        "together to keep the swarm back." },
    { "blackrock", 0, "blackrock mountain,blackrock,ragnaros,molten core,blackwing",
        "Blackrock Mountain is the volcano between the Searing Gorge and the Burning Steppes. Below rule the "
        "Dark Iron dwarves and Ragnaros the Firelord in the Molten Core; above, the Blackrock orcs and "
        "Nefarian's black dragons." },
};
const size_t TopicCount = sizeof(Topics) / sizeof(Topics[0]);

}
