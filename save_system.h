#pragma once
#include <algorithm>

inline void save_manager() {   				
	while (true) {
		Sleep(60000);				
		global_loop_tick++;
		bool onetime = false;

		if (global_loop_tick >= 6) {
			threads.push_back(std::thread(WorldEvents));			
			srand(time(NULL));
			matheventprize = rand() % (50000 - 31000 + 1) + 31000;
			matheventnumber1 = rand() % (265 - 45 + 1) + 45;
			matheventnumber2 = rand() % (265 - 45 + 1) + 45;
			matheventanswer = matheventnumber1 + matheventnumber2;
			global_loop_tick = 0;
			ismathevent = true;
		}
		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
			WorldInfo* world = getPlyersWorld(currentPeer);
			if (world->name == "GROWGANOTH" && !onetime && GrowganothEvent) {
				growganoth2(currentPeer, world, 24000);
				onetime = true;
			}
		}
		LoadEvents();
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (!static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId || static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT" || !static_cast<PlayerInfo*>(currentPeer->data)->isIn) continue;
				if (ismathevent)
				{
					Player::OnConsoleMessage(currentPeer, "`9** `5Matematik Eventi Basladi`9: `3" + to_string(matheventnumber1) + " `5+`3 " + to_string(matheventnumber2) + "`9 = `3? `9Odul: `2" + to_string(matheventprize) + "`9 elmas`3! `o(/a <cevap>).");
					//Player::OnConsoleMessage(currentPeer, "`1Sunucuda yetki satin almak istiyorsan veya daha fazla bilgi almak istiyorsan `9YETKISATISLARI `1dunyasini ziyaret et.");
				}
				if (static_cast<PlayerInfo*>(currentPeer->data)->play_x == static_cast<PlayerInfo*>(currentPeer->data)->x && static_cast<PlayerInfo*>(currentPeer->data)->play_y == static_cast<PlayerInfo*>(currentPeer->data)->y) 
				{
					/*this person is afk to do*/

					static_cast<PlayerInfo*>(currentPeer->data)->afkminutes++;
					if (static_cast<PlayerInfo*>(currentPeer->data)->afkminutes >= 8)
					{
						srand(time(NULL));
						static_cast<PlayerInfo*>(currentPeer->data)->antibotnumber1 = rand() % 50 + 1;
						static_cast<PlayerInfo*>(currentPeer->data)->antibotnumber2 = rand() % 50 + 1;
						static_cast<PlayerInfo*>(currentPeer->data)->isantibotquest = true;
						static_cast<PlayerInfo*>(currentPeer->data)->afkminutes = 0;
						Player::OnDialogRequest(currentPeer, "set_default_color|`o\nadd_label_with_icon|big|`wInsan Misin?``|left|206|\nadd_spacer|small|\nadd_textbox|`oVerilen soruyu dogru bir sekilde cevapla.|\nadd_textbox|" + to_string(static_cast<PlayerInfo*>(currentPeer->data)->antibotnumber1) + " + " + to_string(static_cast<PlayerInfo*>(currentPeer->data)->antibotnumber2) + "|\nadd_text_input|inputantibot|`oCevap:||27|\nend_dialog|antibotsubmit||`wOnayla|\n");
					}

				} else {
					if (static_cast<PlayerInfo*>(currentPeer->data)->afkminutes > 0) static_cast<PlayerInfo*>(currentPeer->data)->afkminutes = 0;
					static_cast<PlayerInfo*>(currentPeer->data)->messageLetters = 0;
					static_cast<PlayerInfo*>(currentPeer->data)->commandsused = 0;
					static_cast<PlayerInfo*>(currentPeer->data)->play_time++;
					static_cast<PlayerInfo*>(currentPeer->data)->play_x = static_cast<PlayerInfo*>(currentPeer->data)->x;
					static_cast<PlayerInfo*>(currentPeer->data)->play_y = static_cast<PlayerInfo*>(currentPeer->data)->y;
					if (std::experimental::filesystem::exists("save/playtimeglobal/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt")) {
						string crt = "";
						ifstream playtimeglobal_write3("save/playtimeglobal/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						playtimeglobal_write3 >> crt;
						playtimeglobal_write3.close();
						ofstream playtimeglobal_write("save/playtimeglobal/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						playtimeglobal_write << to_string(atoi(crt.c_str()) + 1);
						playtimeglobal_write.close(); 
					} else {
						ofstream playtimeglobal_write("save/playtimeglobal/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						playtimeglobal_write << "1";
						playtimeglobal_write.close();
					}
					if (static_cast<PlayerInfo*>(currentPeer->data)->guild != "")
					{
						if (static_cast<PlayerInfo*>(currentPeer->data)->guildxp > 0)
						{
							GuildInfo* guild = guildDB.get_pointer(static_cast<PlayerInfo*>(currentPeer->data)->guild);
							if (guild == NULL)
							{
								Player::OnConsoleMessage(currentPeer, "`4An error occurred while getting guilds information! `1(#1)`4.");
								continue;
							}
							int maxxp = GetMaxGuildXp(guild->level);
							if (guild->xp >= maxxp) continue;
							guild->xp = min(guild->xp + static_cast<PlayerInfo*>(currentPeer->data)->guildxp, maxxp);
							//guild->xp += static_cast<PlayerInfo*>(currentPeer->data)->guildxp;
							static_cast<PlayerInfo*>(currentPeer->data)->guildxp = 0;
							guild->needsave = true;
						}
					}
				}
				if (!GlobalMaintenance && global_loop_tick == 5) {
					save_playerinfo(static_cast<PlayerInfo*>(currentPeer->data));										
				}
			} 
			if (global_loop_tick == 5 || GlobalMaintenance) { /*save all*/
				try {
					for (int i = 0; i < worlds.size(); i++) {
						if (worlds.at(i).name != "EXIT" && worlds.at(i).name != "error") {
							try {

								WorldInfo* info = &worlds.at(i);
								if (info->saved || info->owner == "") continue;
								json j;
								json WorldTiles = json::array();
								int current_update_id = 0;
								const int square = info->width * info->height;
								json WorldDropped = json::array();
								for (int i = 0; i < square; i++) {
									json tile;
									tile["fg"] = info->items.at(i).foreground;
									tile["bg"] = info->items.at(i).background;
									tile["s"] = info->items.at(i).sign;
									tile["r"] = info->items.at(i).flipped;
									tile["int"] = info->items.at(i).intdata;
									tile["l"] = info->items.at(i).label;
									tile["d"] = info->items.at(i).destWorld;
									tile["did"] = info->items.at(i).destId;
									tile["crid"] = info->items.at(i).currId;
									tile["p"] = info->items.at(i).password;
									tile["mid"] = info->items.at(i).mid;
									tile["mc"] = info->items.at(i).mc;
									tile["rm"] = info->items.at(i).rm;
									tile["open"] = info->items.at(i).opened;
									tile["vc"] = info->items.at(i).vcount;
									tile["vd"] = info->items.at(i).vdraw;
									tile["vid"] = info->items.at(i).vid;
									tile["vp"] = info->items.at(i).vprice;
									tile["how"] = info->items.at(i).monitorname;
									tile["mon"] = info->items.at(i).monitoronline;
									tile["spl"] = info->items.at(i).spliced;
									tile["a"] = info->items.at(i).activated;
									tile["sgt"] = info->items.at(i).growtime;
									tile["sfc"] = info->items.at(i).fruitcount;
									tile["w"] = info->items.at(i).water;
									tile["f"] = info->items.at(i).fire;
									tile["red"] = info->items.at(i).red;
									tile["gre"] = info->items.at(i).green;
									tile["blu"] = info->items.at(i).blue;
									tile["es"] = info->items.at(i).evolvestage;
									WorldTiles.push_back(tile);
									tile.clear();
								} for (int i = 0; i < info->droppedItems.size(); i++) {
									json droppedJ;
									droppedJ["c"] = static_cast<BYTE>(info->droppedItems.at(i).count);
									droppedJ["id"] = static_cast<short>(info->droppedItems.at(i).id);
									droppedJ["x"] = info->droppedItems.at(i).x;
									droppedJ["y"] = info->droppedItems.at(i).y;
									droppedJ["uid"] = info->droppedItems.at(i).uid;
									WorldDropped.push_back(droppedJ);
									droppedJ.clear();
								}
								j["name"] = info->name;
								j["owner"] = info->owner;
								string world_admins = "";
								for (int i = 0; i < info->accessed.size(); i++) {
									world_admins += info->accessed.at(i) + "|";
								}
								j["admins"] = world_admins;
								j["nuked"] = info->isNuked;
								j["public"] = info->isPublic;
								j["weather"] = info->weather;
								j["publicBlock"] = info->publicBlock;
								j["silence"] = info->silence;
								j["update_id"] = 0;
								j["disableDrop"] = info->DisableDrop;
								j["category"] = info->category;
								j["rating"] = info->rating;
								j["entrylevel"] = info->entrylevel;
								j["width"] = info->width;
								j["height"] = info->height;
								j["tiles"] = WorldTiles;
								j["dropped"] = WorldDropped;
								j["rainbow"] = info->rainbow;
								ofstream write_player("save/worlds/_" + info->name + ".json");
								write_player << j << std::endl;
								write_player.close();
								j.clear();
								WorldTiles.clear();
								WorldDropped.clear();
								info->saved = true;
							}
							catch (const std::out_of_range& e) {
								std::cout << e.what() << std::endl;
							}
							catch (std::exception& e) {
								cout << e.what() << endl;
							}
						}
					}
					string premium_save = "";
					string platinum_save = "";
					string diamond_save = "";
					for (int i = 0; i < premium_worlds.size(); i++) {
						if (premium_worlds.at(i) == "") continue;
						premium_save += premium_worlds.at(i) + "|";
					} for (int i = 0; i < diamond_worlds.size(); i++) {
						if (diamond_worlds.at(i) == "") continue;
						diamond_save += diamond_worlds.at(i) + "|";
					} for (int i = 0; i < platinum_worlds.size(); i++) {
						if (platinum_worlds.at(i) == "") continue;
						platinum_save += platinum_worlds.at(i) + "|";
					}
					ofstream premium_write("save/premium.txt");
					premium_write << premium_save << std::endl;
					premium_write.close();
					ofstream platinum_write("save/platinum.txt");
					platinum_write << platinum_save << std::endl;
					platinum_write.close();
					ofstream diamond_write("save/diamond.txt");
					diamond_write << diamond_save << std::endl;
					diamond_write.close();
				}
				catch (const std::out_of_range& e) {
					std::cout << e.what() << std::endl;
					cout << "save world error" << endl;
				}
				catch (std::exception& e) {
					cout << e.what() << endl;
					cout << "save world error" << endl;
				}

				try {
					for (int i = 0; i < guilds.size(); i++)
					{
						GuildInfo* info = &guilds.at(i);
						if (!info->needsave) continue;
						json j;
						json members = json::array();
						j["level"] = info->level;
						j["xp"] = info->xp;
						j["background"] = info->background;
						j["foreground"] = info->foreground;
						j["statement"] = info->statement;
						j["leader"] = info->leader;
						j["name"] = info->name;
						j["world"] = info->world;
						j["logs"] = info->logs;
						j["notebook"] = info->notebook;
						j["worldlockidbeforeguildcreation"] = info->worldlockidbeforeguildcreation;
						j["accesslockCoLeader"] = info->accesslockCoLeader;
						j["accesslockCoLeaderAndElder"] = info->accesslockCoLeaderAndElder;
						j["accesslockAllMembers"] = info->accesslockAllMembers;
						for (int i = 0; i < info->members.size(); i++)
						{
							json member;
							member["growid"] = info->members.at(i).growid;
							member["totalxp"] = info->members.at(i).totalxp;
							member["joinedGuildTime"] = info->members.at(i).joinedGuildTime;
							member["rank"] = info->members.at(i).rank;
							member["guildcheckbox_public"] = info->members.at(i).guildcheckbox_public;
							member["guildcheckbox_notifications"] = info->members.at(i).guildcheckbox_notifications;
							members.push_back(member);
							member.clear();
						}
						j["members"] = members;
						ofstream write_guild("save/guilds/_" + stringtoupper(info->name) + ".json");
						write_guild << j << std::endl;
						write_guild.close();
						j.clear();
						members.clear();
						info->needsave = false;
					}
				}
				catch (const std::out_of_range& e) {
					std::cout << e.what() << std::endl;
					cout << "save guild error" << endl;
				}
				catch (std::exception& e) {
					cout << e.what() << endl;
					cout << "save guild error" << endl;
				}

				if (GlobalMaintenance) {
					SendConsole("Worlds and Guilds are now saved! server shutted down as requested by one player", "INFO");
					//exit(0);
				}
		}
	}
}
