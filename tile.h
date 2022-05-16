#pragma once
#include "player.h"
#include "world.h"
#include "trade_system.h"
#include "discord_webhook.h"

inline void playerRespawn(WorldInfo* world, ENetPeer* peer, const bool isDeadByTile) {					
	if (static_cast<PlayerInfo*>(peer->data)->trade) end_trade(peer);
	if (static_cast<PlayerInfo*>(peer->data)->Fishing) {
		static_cast<PlayerInfo*>(peer->data)->TriggerFish = false;
		static_cast<PlayerInfo*>(peer->data)->FishPosX = 0;
		static_cast<PlayerInfo*>(peer->data)->FishPosY = 0;
		static_cast<PlayerInfo*>(peer->data)->Fishing = false;
		send_state(peer);
		Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wSit perfectly when fishing!", 0, false);
		Player::OnSetPos(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y);
	}
	auto netID = static_cast<PlayerInfo*>(peer->data)->netID;
	if (isDeadByTile == false) {
		Player::OnKilled(peer, static_cast<PlayerInfo*>(peer->data)->netID);
	}
	auto p2x = packetEnd(appendInt(appendString(createPacket(), "OnSetFreezeState"), 0));
	memcpy(p2x.data + 8, &netID, 4);
	auto respawnTimeout = 2500;
	auto deathFlag = 0x19;
	memcpy(p2x.data + 24, &respawnTimeout, 4);
	memcpy(p2x.data + 56, &deathFlag, 4);
	const auto packet2x = enet_packet_create(p2x.data, p2x.len, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet2x);
	delete p2x.data;
	const auto p5 = packetEnd(appendInt(appendString(createPacket(), "OnSetFreezeState"), 2));
	memcpy(p5.data + 8, &(static_cast<PlayerInfo*>(peer->data)->netID), 4);
	const auto packet5 = enet_packet_create(p5.data, p5.len, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet5);
	GamePacket p2;
	static_cast<PlayerInfo*>(peer->data)->disableanticheat = true;
	static_cast<PlayerInfo*>(peer->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 3;
	auto x = 3040;
	auto y = 736;
	try {
		for (auto i = 0; i < world->width * world->height; i++) {
			if (world->items.at(i).foreground == 6) {
				x = (i % world->width) * 32;
				y = (i / world->width) * 32;
				break;
			}
		}
	} catch(const std::out_of_range& e) {
		std::cout << e.what() << std::endl;
	} 
	if (static_cast<PlayerInfo*>(peer->data)->ischeck) {
		p2 = packetEnd(appendFloat(appendString(createPacket(), "OnSetPos"), static_cast<PlayerInfo*>(peer->data)->checkx, static_cast<PlayerInfo*>(peer->data)->checky));
	} else {
		p2 = packetEnd(appendFloat(appendString(createPacket(), "OnSetPos"), x, y));
	}
	memcpy(p2.data + 8, &(static_cast<PlayerInfo*>(peer->data)->netID), 4);
	respawnTimeout = 2500;
	memcpy(p2.data + 24, &respawnTimeout, 4);
	memcpy(p2.data + 56, &deathFlag, 4);
	const auto packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet2);
	delete p2.data;
	auto p2a = packetEnd(appendString(appendString(createPacket(), "OnPlayPositioned"), "audio/teleport.wav"));
	memcpy(p2a.data + 8, &netID, 4);
	respawnTimeout = 2500;
	memcpy(p2a.data + 24, &respawnTimeout, 4);
	memcpy(p2a.data + 56, &deathFlag, 4);
	const auto packet2a = enet_packet_create(p2a.data, p2a.len, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, 0, packet2a);
	delete p2a.data;

	static_cast<PlayerInfo*>(peer->data)->spikehack = false;
	static_cast<PlayerInfo*>(peer->data)->ghosthackcheck = false;
	static_cast<PlayerInfo*>(peer->data)->lavahack = false;
	static_cast<PlayerInfo*>(peer->data)->lastx = 0; //antihack last pos
	static_cast<PlayerInfo*>(peer->data)->lasty = 0; // antihack last pos
}


inline void sendWorld(ENetPeer* peer, WorldInfo* worldInfo) {
	try {
		auto zero = 0;
		static_cast<PlayerInfo*>(peer->data)->joinClothesUpdated = false;
		string asdf = "0400000004A7379237BB2509E8E0EC04F8720B050000000000000000FBBB0000010000007D920100FDFDFDFD04000000040000000000000000000000070000000000";
		string worldName = worldInfo->name;
		auto xSize = worldInfo->width;
		auto ySize = worldInfo->height;
		auto square = xSize * ySize;
		auto nameLen = static_cast<uint16_t>(worldName.length());
		int payloadLen = asdf.length() / 2;
		auto dataLen = payloadLen + 2 + nameLen + 12 + (square * 8) + 4 + 100;
		auto offsetData = dataLen - 100;
		int allocMem = payloadLen + 2 + nameLen + 12 + (square * 8) + 4 + 1640000 + 100 + (worldInfo->droppedItems.size() * 20);
		auto data = new BYTE[allocMem];
		memset(data, 0, allocMem);
		for (auto i = 0; i < asdf.length(); i += 2) {
			char x = ch2n(asdf.at(i));
			x = x << 4;
			x += ch2n(asdf[i + 1]);
			memcpy(data + (i / 2), &x, 1);
		}
		uint16_t item = 0;
		auto smth = 0;
		for (auto i = 0; i < square * 8; i += 4) memcpy(data + payloadLen + i + 14 + nameLen, &zero, 4);
		for (auto i = 0; i < square * 8; i += 8) memcpy(data + payloadLen + i + 14 + nameLen, &item, 2);
		memcpy(data + payloadLen, &nameLen, 2);
		memcpy(data + payloadLen + 2, worldName.c_str(), nameLen);
		memcpy(data + payloadLen + 2 + nameLen, &xSize, 4);
		memcpy(data + payloadLen + 6 + nameLen, &ySize, 4);
		memcpy(data + payloadLen + 10 + nameLen, &square, 4);
		BYTE* blockPtr = data + payloadLen + 14 + nameLen;
		auto sizeofblockstruct = 8;
		for (auto i = 0; i < square; i++) {
			int tile = worldInfo->items.at(i).foreground;
			sizeofblockstruct = 8;
			auto type = 0x00000000;
			if (worldInfo->items.at(i).activated) type |= 0x00400000;
			if (worldInfo->items.at(i).flipped) type |= 0x00250000;
			if (worldInfo->items.at(i).water) type |= 0x04000000;
			if (worldInfo->items.at(i).glue) type |= 0x08000000;
			if (worldInfo->items.at(i).fire) type |= 0x10000000;
			if (worldInfo->items.at(i).red) type |= 0x25000000;
			if (worldInfo->items.at(i).green) type |= 0x40000000;
			if (worldInfo->items.at(i).blue) type |= 0x80000000;
			switch (tile) {
				case 6:
				{
					memcpy(blockPtr, &tile, 2);
					memcpy(blockPtr + 4, &type, 4);
					BYTE btype = 1;
					memcpy(blockPtr + 8, &btype, 1);
					string doorText = "EXIT";
					auto doorTextChars = doorText.c_str();
					auto length = static_cast<short>(doorText.size());
					memcpy(blockPtr + 9, &length, 2);
					memcpy(blockPtr + 11, doorTextChars, length);
					sizeofblockstruct += 4 + length;
					dataLen += 4 + length;
					break;
				}
				case 2946:
				{
					memcpy(blockPtr, &worldInfo->items.at(i).foreground, 2);
					memcpy(blockPtr + 4, &type, 4);
					BYTE btype = 0x17;
					memcpy(blockPtr + 8, &btype, 1);
					auto item1 = worldInfo->items.at(i).intdata;
					memcpy(blockPtr + 9, &item1, 4);
					sizeofblockstruct += 5;
					dataLen += 5;
					break;
				}
				case 1632: case 8196: case 10450: case 5116: case 6414: case 6212: case 3044: case 1636: case 1008: case 2798: case 1044: case 872: case 866: case 3888: case 928:
				{
					BYTE btype = 9;
					auto timeIntoGrowth = getItemDef(worldInfo->items.at(i).foreground).growTime - calcBanDuration(worldInfo->items.at(i).growtime);
					memcpy(blockPtr, &worldInfo->items.at(i).foreground, 2);
					memcpy(blockPtr + 4, &type, 4);
					memcpy(blockPtr + 8, &btype, 1);
					memcpy(blockPtr + 9, &timeIntoGrowth, 4);
					sizeofblockstruct += 5;
					dataLen += 5;
					break;
				}
				default:
				{
					if ((worldInfo->items.at(i).foreground == 0) || (worldInfo->items.at(i).foreground == 2) || (worldInfo->items.at(i).foreground == 8) || (worldInfo->items.at(i).foreground == 100) || (worldInfo->items.at(i).foreground == 4) || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::FOREGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::BACKGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::TOGGLE_FOREGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::CHEMICAL_COMBINER || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::CHEST || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::SWITCH_BLOCK) {
						memcpy(blockPtr, &worldInfo->items.at(i).foreground, 2);
						memcpy(blockPtr + 4, &type, 4);
					} else {
						memcpy(blockPtr, &zero, 2);
					}
					break;
				}
			}
			memcpy(blockPtr + 2, &worldInfo->items.at(i).background, 2);
			blockPtr += sizeofblockstruct;
		}
		dataLen += 8;
		int itemcount = worldInfo->droppedItems.size();
		auto itemuid = worldInfo->droppedCount;
		memcpy(blockPtr, &itemcount, 4);
		memcpy(blockPtr + 4, &itemuid, 4);
		blockPtr += 8;
		auto iteminfosize = 16;
		for (auto i = 0; i < itemcount; i++) {
			auto item1 = worldInfo->droppedItems.at(i).id;
			auto count = worldInfo->droppedItems.at(i).count;
			auto uid = worldInfo->droppedItems.at(i).uid + 1;
			auto x = static_cast<float>(worldInfo->droppedItems.at(i).x);
			auto y = static_cast<float>(worldInfo->droppedItems.at(i).y);
			memcpy(blockPtr, &item1, 2);
			memcpy(blockPtr + 2, &x, 4);
			memcpy(blockPtr + 6, &y, 4);
			memcpy(blockPtr + 10, &count, 2);
			memcpy(blockPtr + 12, &uid, 4);
			blockPtr += iteminfosize;
			dataLen += iteminfosize;
		}
		dataLen += 100;
		blockPtr += 4;
		memcpy(blockPtr, &worldInfo->weather, 4);
		blockPtr += 4;
		offsetData = dataLen - 100;
		if (dataLen > 1640000) {
			SendConsole("World dataLen is too big to handle " + to_string(dataLen) + " in world " + worldInfo->name, "ERROR");
			return;
		}
		auto data2 = new BYTE[101];
		memset(data2, 0, 101);
		memcpy(data2 + 0, &zero, 4);
		auto weather = worldInfo->weather;
		memcpy(data2 + 4, &weather, 4);
		memcpy(data + dataLen - 4, &smth, 4);
		auto packet2 = enet_packet_create(data, dataLen, ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(peer, 0, packet2);
		static_cast<PlayerInfo*>(peer->data)->currentWorld = worldInfo->name;
		for (auto i = 0; i < square; i++)
		{
			switch (worldInfo->items.at(i).foreground)
			{
				case 3528: //Painting Easel
				{
					int xx = i % xSize, yy = i / xSize;
					SendCanvasData(peer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, xx, yy, worldInfo->items.at(i).intdata, getItemDef(worldInfo->items.at(i).intdata).name);
					break;
				}
				case 3818: //Portrait
				{
					int xx = i % xSize, yy = i / xSize;
					SendCanvasData(peer, worldInfo->items[i].foreground, worldInfo->items[i].background, xx, yy, worldInfo->items[i].intdata, getItemDef(worldInfo->items[i].intdata).name);
					break;
				}
				case 6952:
				{
					if (worldInfo->items.at(i).mid != 0) send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, 1, false, false, worldInfo->items.at(i).background);
					else send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, 0, false, false, worldInfo->items.at(i).background);
					break;
				}
				case 6954:
				{
					if (worldInfo->items.at(i).mid > 0 && worldInfo->items.at(i).mc > 0) {
						send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, 1, true, true, worldInfo->items.at(i).background);
					} else {
						send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, -1, true, true, worldInfo->items.at(i).background);
					}
					break;
				}
				case 5638: case 6946: case 6948: //MAGPLANTS
				{
					if (worldInfo->items.at(i).mc <= 0) {
						send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, 0, true, true, worldInfo->items.at(i).background);
					}
					else if (worldInfo->items.at(i).mc >= 5000 && worldInfo->items.at(i).mid != 112) {
						send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, -1, true, true, worldInfo->items.at(i).background);
					}
					else {
						send_item_sucker(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).mid, 1, true, true, worldInfo->items.at(i).background);
					}
					break;
				}
				case 2978: case 9268: //Vending Machine
				{
					auto islocks = false;
					if (worldInfo->items.at(i).vdraw >= 1) {
						islocks = true;
					}
					if (worldInfo->items.at(i).vcount == 0 && worldInfo->items.at(i).vprice == 0 && worldInfo->items.at(i).vid != 0) {
						UpdateVend(peer, i % worldInfo->width, i / worldInfo->width, 0, islocks, 0, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, worldInfo->items.at(i).opened);
					}
					else if (worldInfo->items.at(i).opened && worldInfo->items.at(i).vcount < worldInfo->items.at(i).vprice) {
						UpdateVend(peer, i % worldInfo->width, i / worldInfo->width, 0, islocks, worldInfo->items.at(i).vprice, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, worldInfo->items.at(i).opened);
					}
					else UpdateVend(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).vid, islocks, worldInfo->items.at(i).vprice, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, worldInfo->items.at(i).opened);
					break;
				}
				case 1420: case 6214: //Mannequin
				{
					bool sent = false;
					auto ismannequin = std::experimental::filesystem::exists("save/mannequin/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
					if (ismannequin)
					{
						sent = true;
						json j;
						ifstream fs("save/mannequin/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
						fs >> j;
						fs.close();
						auto x = i % worldInfo->width;
						auto y = i / worldInfo->width;
						//0x00250000
						updateMannequin(peer, worldInfo->items.at(i).foreground, x, y, worldInfo->items.at(i).background, worldInfo->items.at(i).sign, atoi(j["clothHair"].get<string>().c_str()), atoi(j["clothHead"].get<string>().c_str()), atoi(j["clothMask"].get<string>().c_str()), atoi(j["clothHand"].get<string>().c_str()), atoi(j["clothNeck"].get<string>().c_str()), atoi(j["clothShirt"].get<string>().c_str()), atoi(j["clothPants"].get<string>().c_str()), atoi(j["clothFeet"].get<string>().c_str()), atoi(j["clothBack"].get<string>().c_str()), false, 0);
					}
					if (!sent) {
							PlayerMoving moving{};
							moving.packetType = 0x3;
							moving.characterState = 0x0;
							moving.x = i % worldInfo->width;
							moving.y = static_cast<float>(i) / worldInfo->height;
							moving.punchX = i % worldInfo->width;
							moving.punchY = i / worldInfo->width;
							moving.XSpeed = 0;
							moving.YSpeed = 0;
							moving.netID = -1;
							moving.plantingTree = worldInfo->items.at(i).foreground;
							SendPacketRaw(4, packPlayerMoving(&moving), 56, nullptr, peer, ENET_PACKET_FLAG_RELIABLE);
						}
					break;
				}
				case 1006: //Blue Mailbox
				{
					bool sent = false;
					auto isbluemail = std::experimental::filesystem::exists("save/bluemailbox/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
					if (isbluemail)
					{
						ifstream ifff("save/bluemailbox/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
						json basic_json;
						ifff >> basic_json;
						ifff.close();
						if (basic_json["inmail"] > 0)
						{
							/*removed because started to crash when entering worlds*/
							//sent = true;
							auto x = i % worldInfo->width;
							auto y = i / worldInfo->width;
							SendItemPacket(peer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, x, y, 1);
						}
					}
					if (!sent) {
							PlayerMoving moving{};
							moving.packetType = 0x3;
							moving.characterState = 0x0;
							moving.x = i % worldInfo->width;
							moving.y = static_cast<float>(i) / worldInfo->height;
							moving.punchX = i % worldInfo->width;
							moving.punchY = i / worldInfo->width;
							moving.XSpeed = 0;
							moving.YSpeed = 0;
							moving.netID = -1;
							moving.plantingTree = worldInfo->items.at(i).foreground;
							SendPacketRaw(4, packPlayerMoving(&moving), 56, nullptr, peer, ENET_PACKET_FLAG_RELIABLE);
						}
					break;
				}
				case 1240: //Heart Monitor
				{
					bool found = false;
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (static_cast<PlayerInfo*>(currentPeer->data)->displayName == worldInfo->items.at(i).monitorname) {
							found = true;
							sendHMonitor(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).monitorname, true, worldInfo->items.at(i).background);
							break;
						}
					}
					if (!found) sendHMonitor(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).monitorname, false, worldInfo->items.at(i).background);
					break;
				}
				case 0: case 6: case 2946: case 8196: case 10450: case 1008: case 866: case 6414: case 6212: case 5116: case 4858: case 3888: case 3044: case 2798: case 1632: case 1636: case 1044: case 928: case 872: case 4: case 2: case 8: case 100:
				{
					/*if (worldInfo->items.at(i).destroy && worldInfo->items.at(i).foreground != 0) {
						for (int asd = 0; asd < getItemDef(worldInfo->items.at(i).foreground).breakHits; asd++) {
							if (worldInfo->items.at(i).foreground != 0) {
								sendTileUpdate(i % worldInfo->width, i / worldInfo->width, 18, -1, peer, worldInfo);
								worldInfo->items.at(i).foreground = 0;
							}
						}
						worldInfo->items.at(i).destroy = false;
					}*/
					break;
				}
				default:
				{
					if (worldInfo->items.at(i).foreground == 3694 && worldInfo->items.at(i).activated) {
						sendHeatwave(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).vid, worldInfo->items.at(i).vprice, worldInfo->items.at(i).vcount);
						Player::OnSetCurrentWeather(peer, 28);
						break;
					}
					else if (worldInfo->items.at(i).foreground == 3832 && worldInfo->items.at(i).activated) {
						sendStuffweather(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).intdata, worldInfo->items.at(i).mc, worldInfo->items.at(i).rm, worldInfo->items.at(i).opened);
						Player::OnSetCurrentWeather(peer, 29);
						break;
					} 
					else if (worldInfo->items.at(i).foreground == 5000 && worldInfo->items.at(i).activated) {
						sendBackground(peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).intdata);
						Player::OnSetCurrentWeather(peer, 34);
						break;
					}
					if (worldInfo->items.at(i).foreground == 3616 && worldInfo->items.at(i).activated) {
						worldInfo->isPineappleGuard == true;
						break;
					}
					/*else if (worldInfo->items.at(i).destroy && worldInfo->items.at(i).foreground != 0) {
						for (int asd = 0; asd <= getItemDef(worldInfo->items.at(i).foreground).breakHits; asd++) {
							if (worldInfo->items.at(i).foreground != 0) {
								sendTileUpdate(i % worldInfo->width, i / worldInfo->width, 18, -1, peer, worldInfo);
								worldInfo->items.at(i).foreground = 0;
							}
						}
						worldInfo->items.at(i).destroy = false;
					}*/
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::SIGN || worldInfo->items.at(i).foreground == 1420 || worldInfo->items.at(i).foreground == 6124) {
						UpdateMessageVisuals(peer, worldInfo->items.at(i).foreground, i% worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).sign, worldInfo->items.at(i).background);
						break;
					}
					/*else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::FOREGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::BACKGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::CHEST || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::SWITCH_BLOCK || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::TOGGLE_FOREGROUND || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::CHEMICAL_COMBINER || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::SIGN || getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::PORTAL) {
						break;
					}*/
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::DONATION) {
						bool sent = false;
						auto isdbox = std::experimental::filesystem::exists("save/donationboxes/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
						if (isdbox)
						{
							ifstream ifff("save/donationboxes/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "/X" + std::to_string(i) + ".json");
							json basic_json;
							ifff >> basic_json;
							ifff.close();
							if (basic_json["donated"] > 0) {
								//sent = true;
								SendItemPacket(peer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, i % worldInfo->width, i / worldInfo->width, 1);
							}
						}
						if (!sent) {
							PlayerMoving moving{};
							moving.packetType = 0x3;
							moving.characterState = 0x0;
							moving.x = i % worldInfo->width;
							moving.y = static_cast<float>(i) / worldInfo->height;
							moving.punchX = i % worldInfo->width;
							moving.punchY = i / worldInfo->width;
							moving.XSpeed = 0;
							moving.YSpeed = 0;
							moving.netID = -1;
							moving.plantingTree = worldInfo->items.at(i).foreground;
							SendPacketRaw(4, packPlayerMoving(&moving), 56, nullptr, peer, ENET_PACKET_FLAG_RELIABLE);
						}
						break;
					}
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::SEED) {
						int growTimeSeed = getItemDef(worldInfo->items.at(i).foreground - 1).rarity * getItemDef(worldInfo->items.at(i).foreground - 1).rarity * getItemDef(worldInfo->items.at(i).foreground - 1).rarity;
						growTimeSeed += 30 * getItemDef(worldInfo->items.at(i).foreground - 1).rarity;
						UpdateTreeVisuals(peer, worldInfo->items.at(i).foreground, i % xSize, i / xSize, worldInfo->items.at(i).background, worldInfo->items.at(i).fruitcount, growTimeSeed - calcBanDuration(worldInfo->items.at(i).growtime), false, 0);
					}
					else if (worldInfo->items.at(i).foreground == 3798)
					{
						if (isDev(peer) || isWorldOwner(peer, worldInfo) || isWorldAdmin(peer, worldInfo) || isAdminVipEntrance(peer, worldInfo, i % worldInfo->width, i / worldInfo->width)) {
							update_entrance(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, true, worldInfo->items.at(i).background);
						}
						else
						{
							update_entrance(peer, worldInfo->items.at(i).foreground, i% worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).opened, worldInfo->items.at(i).background);
						}
					}
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::GATEWAY) {
						if (isDev(peer) || isWorldOwner(peer, worldInfo) || isWorldAdmin(peer, worldInfo)) {
							update_entrance(peer, worldInfo->items.at(i).foreground, i% worldInfo->width, i / worldInfo->width, true, worldInfo->items.at(i).background);
						}
						else {
							update_entrance(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).opened, worldInfo->items.at(i).background);
						}
					}
					else if (getItemDef(worldInfo->items.at(i).foreground).properties == Property_MultiFacing) {
						UpdateBlockState(peer, i % worldInfo->width, i / worldInfo->width, true, worldInfo);
						break;
					}
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::DOOR) {
						updateDoor(peer, worldInfo->items.at(i).foreground, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).background, worldInfo->items.at(i).label == "" ? (worldInfo->items.at(i).destId == "" ? worldInfo->items.at(i).destWorld : worldInfo->items.at(i).destWorld + "...") : worldInfo->items.at(i).label, !worldInfo->items.at(i).opened, false);
					}
					else if (getItemDef(worldInfo->items.at(i).foreground).blockType == BlockTypes::LOCK) {
						if (worldInfo->items.at(i).foreground == 202 || worldInfo->items.at(i).foreground == 204 || worldInfo->items.at(i).foreground == 206 || worldInfo->items.at(i).foreground == 4994) {
							if (worldInfo->items.at(i).monitorname == static_cast<PlayerInfo*>(peer->data)->rawName) apply_lock_packet(worldInfo, peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).foreground, static_cast<PlayerInfo*>(peer->data)->netID);
							else if (worldInfo->items.at(i).opened) apply_lock_packet(worldInfo, peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).foreground, -3);
							else apply_lock_packet(worldInfo, peer, i % worldInfo->width, i / worldInfo->width, worldInfo->items.at(i).foreground, -1);
						}
						else if (isWorldOwner(peer, worldInfo) && worldInfo->items.at(i).foreground != 5814) send_tile_data(peer, i % worldInfo->width, i / worldInfo->width, 0x10, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, lock_tile_datas(0x20, static_cast<PlayerInfo*>(peer->data)->netID, 0, 0, false, 100));
						else if (isWorldAdmin(peer, worldInfo) && worldInfo->items.at(i).foreground != 5814) send_tile_data(peer, i % worldInfo->width, i / worldInfo->width, 0x10, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, lock_tile_datas(0x20, static_cast<PlayerInfo*>(peer->data)->netID, 0, 0, true, 100));
						else {
							PlayerMoving moving{};
							moving.packetType = 0x3;
							moving.characterState = 0x0;
							moving.x = i % worldInfo->width;
							moving.y = static_cast<float>(i) / worldInfo->height;
							moving.punchX = i % worldInfo->width;
							moving.punchY = i / worldInfo->width;
							moving.XSpeed = 0;
							moving.YSpeed = 0;
							moving.netID = -1;
							moving.plantingTree = worldInfo->items.at(i).foreground;
							SendPacketRaw(4, packPlayerMoving(&moving), 56, nullptr, peer, ENET_PACKET_FLAG_RELIABLE);
						}
						if (worldInfo->items.at(i).foreground == 4802 && worldInfo->rainbow && isWorldOwner(peer, worldInfo)) {
							send_rainbow_shit_data(peer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, i % worldInfo->width, i / worldInfo->width, true, static_cast<PlayerInfo*>(peer->data)->netID);
							for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer) && !isWorldOwner(currentPeer, worldInfo)) {
									send_rainbow_shit_data(currentPeer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, i % worldInfo->width, i / worldInfo->width, true, static_cast<PlayerInfo*>(peer->data)->netID);
								}
							}
						} else if (worldInfo->items.at(i).foreground == 4802 && !isWorldOwner(peer, worldInfo)) {
							for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer) && isWorldOwner(currentPeer, worldInfo)) {
									send_rainbow_shit_data(peer, worldInfo->items.at(i).foreground, worldInfo->items.at(i).background, i % worldInfo->width, i / worldInfo->width, true, static_cast<PlayerInfo*>(currentPeer->data)->netID);
									break;
								}
							}
						}
					}
					else {
						PlayerMoving moving{};
						moving.packetType = 0x3;
						moving.characterState = 0x0;
						moving.x = i % worldInfo->width;
						moving.y = static_cast<float>(i) / worldInfo->height;
						moving.punchX = i % worldInfo->width;
						moving.punchY = i / worldInfo->width;
						moving.XSpeed = 0;
						moving.YSpeed = 0;
						moving.netID = -1;
						moving.plantingTree = worldInfo->items.at(i).foreground;
						SendPacketRaw(4, packPlayerMoving(&moving), 56, nullptr, peer, ENET_PACKET_FLAG_RELIABLE);
					}
					break;
				}
			}
		}
		static_cast<PlayerInfo*>(peer->data)->lastnormalworld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
		ifstream ifs("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
		string content((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));
		auto gembux = atoi(content.c_str());
		Player::OnSetBux(peer, gembux, 1);
		Player::PlayAudio(peer, "audio/door_open.wav", 0);
		delete[] data;
		delete[] data2;
	} catch(const std::out_of_range& e) {
		std::cout << e.what() << std::endl;
	} 
}

inline void joinWorld(WorldInfo info, ENetPeer* peer, string act, int x2, int y2) {
	try {
		string upsd = act;
		transform(upsd.begin(), upsd.end(), upsd.begin(), ::toupper);

		if (act == "GAME1" && game1status == true)
		{
			Player::OnConsoleMessage(peer, "`oOyun zaten baslamis.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "GAME1BACKUP" && game1status == true)
		{
			Player::OnConsoleMessage(peer, "`oErisim engellendi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}

		if (static_cast<PlayerInfo*>(peer->data)->isCursed) act = "CEHENNEM";
		if (static_cast<PlayerInfo*>(peer->data)->isHospitalized) act = "BASLA";
		if (act.length() > 24 || act.length() < 0) {
			Player::OnConsoleMessage(peer, "`4Uzgunuz, ama 24 karakterden uzun olan dunyalara destek vermiyoruz!");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		else if (static_cast<PlayerInfo*>(peer->data)->haveGrowId) {
			static_cast<PlayerInfo*>(peer->data)->checkteleportspeedflyhacks = false;
			DailyRewardCheck(peer);
			auto iscontains = false;
			SearchInventoryItem(peer, 6336, 1, iscontains);
			if (!iscontains) {
				auto success = true;
				SaveItemMoreTimes(6336, 1, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " from system");
			}
			/*magplant*/
			auto iscontainss = false;
			SearchInventoryItem(peer, 5640, 1, iscontainss);
			if (iscontainss) {
				static_cast<PlayerInfo*>(peer->data)->magplantitemid = 0;
				RemoveInventoryItem(5640, 1, peer, true);
			}
			/*worldkey, guildkey*/
			iscontainss = false; 
			SearchInventoryItem(peer, 1424, 1, iscontainss);
			if (iscontainss) {
				RemoveInventoryItem(1424, 1, peer, true);
			}
			SearchInventoryItem(peer, 5816, 1, iscontainss);
			if (iscontainss) {
				RemoveInventoryItem(5816, 1, peer, true);
			}
			string check_it = "";
			ifstream read_winner("save/win.txt");
			read_winner >> check_it;
			read_winner.close();
			if (check_it != "") {
				auto ex = explode("|", check_it);
				string the_guy = ex.at(0);
				int the_item = atoi(ex.at(1).c_str());
				toUpperCase(the_guy);
				string peer_name = static_cast<PlayerInfo*>(peer->data)->rawName; 
				toUpperCase(peer_name);
				if (peer_name == the_guy) {
					ofstream write_winner("save/win.txt");
					write_winner << "";
					write_winner.close();
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "You won a `5" + getItemDef(the_item).name + "`` for most playtime!!", 0, false);
					Player::OnConsoleMessage(peer, "You won a `5" + getItemDef(the_item).name + "`` for most playtime!!");
					auto success = true;
					SaveItemMoreTimes(the_item, 1, peer, success);
				}
			}

		}
		static_cast<PlayerInfo*>(peer->data)->netID = cId;
		int x = 3040;
		int y = 736;
		string Definitions = " `0[";
		if (info.category != "None") {
			if (info.category == "Guild") {
				Definitions += "`2KLAN";
			}
			else {
				Definitions += "`9" + info.category;
			}
		}
		if (std::find(premium_worlds.begin(), premium_worlds.end(), info.name) != premium_worlds.end()) {
			if (Definitions == " `0[") Definitions += "`$Premiyum";
			else Definitions += "``, `$Premium";
		}
		else if (std::find(diamond_worlds.begin(), diamond_worlds.end(), info.name) != diamond_worlds.end()) {
			if (Definitions == " `0[") Definitions += "`cElmas";
			else Definitions += "``, `cDiamond";
		}
		else if (std::find(platinum_worlds.begin(), platinum_worlds.end(), info.name) != platinum_worlds.end()) {
			if (Definitions == " `0[") Definitions += "`5Zumrut";
			else Definitions += "``, `5Zumrut";
		}
		static_cast<PlayerInfo*>(peer->data)->worldContainsActivatedAntiGravity = false;
		for (auto jss = 0; jss < info.width * info.height; jss++) {
			if (jammers) {
				if (info.items.at(jss).foreground == 226 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`4GIZLI";
					else Definitions += "``, `4GIZLI";
				}
				if (info.items.at(jss).foreground == 1276 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`2YUMRUKYOK";
					else Definitions += "``, `2YUMRUKYOK";
				}
				if (info.items.at(jss).foreground == 1278 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`2IMMUNE";
					else Definitions += "``, `2IMMUNE";
				}
				if (info.items.at(jss).foreground == 4992 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`2YERCEKIMSIZ";
					else Definitions += "``, `2YERCEKIMSIZ";
					static_cast<PlayerInfo*>(peer->data)->worldContainsActivatedAntiGravity = true;
				}
				if (info.items.at(jss).foreground == 7560 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`2EVENTYOK";
					else Definitions += "``, `2EVENTYOK";
				}
				if (info.items.at(jss).foreground == 9522 && info.items.at(jss).activated) {
					if (Definitions == " `0[") Definitions += "`#FIZIKLERAKTIF";
					else Definitions += "``, `#FIZIKLERAKTIF";
				}
			}
			if (info.items.at(jss).foreground == 6) {
				x = (jss % info.width) * 32;
				y = (jss / info.width) * 32;
				if (!jammers) break;
			}
		}
		if (info.isNuked) {
			if (Definitions == " `0[") Definitions += "`4Dunya banlanmis, normal oyuncular tarafindan erisilemez";
			else Definitions += "``, `4Dunya banlanmis, normal oyuncular tarafindan erisilemez";
		}
		Definitions += "`0]";
		if (Definitions == " `0[`0]") Definitions = "";
		Player::OnConsoleMessage(peer, "`w" + info.name + "``" + Definitions + " `oworldune giris yaptin. Bu Dunyada senden baska `w" + to_string(getPlayersCountInWorld(info.name)) + "`` kisi var, sunucuda ise toplamda `w" + GetPlayerCountServer() + "`` aktif oyuncu var.");		size_t pos;
		while ((pos = static_cast<PlayerInfo*>(peer->data)->displayName.find("`2")) != string::npos) {
			static_cast<PlayerInfo*>(peer->data)->displayName.replace(pos, 2, "");
		}
		if (info.owner != "") {
			try {
				ifstream read_player("save/players/_" + info.owner + ".json");
				if (!read_player.is_open()) {
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				string username = j["nick"];
				int adminLevel = j["adminLevel"];
				if (username == "") {
					username = role_prefix.at(adminLevel) + info.owner;
				} 
				if (info.owner == static_cast<PlayerInfo*>(peer->data)->rawName || username == static_cast<PlayerInfo*>(peer->data)->displayName || isWorldAdmin(peer, info)) {
					Player::OnConsoleMessage(peer, "`5[`w" + info.name + "`$ Dunyanin Sahibi`o  " + username + " `w(`2YETKI SAGLANDI`w)`5]");
					if (info.notification != "") {
						Player::OnConsoleMessage(peer, info.notification);
						info.notification = "";
					}
					if (info.owner == static_cast<PlayerInfo*>(peer->data)->rawName || username == static_cast<PlayerInfo*>(peer->data)->displayName) {
						if (static_cast<PlayerInfo*>(peer->data)->displayName.find("`") != string::npos) {} else {
							static_cast<PlayerInfo*>(peer->data)->displayName = "`2" + static_cast<PlayerInfo*>(peer->data)->displayName;
							Player::OnNameChanged(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName);
						}
					}
				}
				else if (info.publicBlock != -1 && info.publicBlock != 0) {
					Player::OnConsoleMessage(peer, "`5[`w" + info.name + "`$ Dunyanin Sahibi`o " + username + " ``(`^BFG``)`5]");
				} else {
					Player::OnConsoleMessage(peer, "`5[`w" + info.name + "`$ Dunyanin Sahibi`o " + username + "`5]");
				}
				if (isWorldAdmin(peer, info)) {									
					size_t pos;
					while ((pos = static_cast<PlayerInfo*>(peer->data)->displayName.find("`w")) != string::npos) {
						static_cast<PlayerInfo*>(peer->data)->displayName.replace(pos, 2, "");
					}
					static_cast<PlayerInfo*>(peer->data)->displayName = "`^" + static_cast<PlayerInfo*>(peer->data)->displayName;
					Player::OnNameChanged(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName);
				}
				else if (info.owner != static_cast<PlayerInfo*>(peer->data)->rawName && username != static_cast<PlayerInfo*>(peer->data)->displayName) {
					if (static_cast<PlayerInfo*>(peer->data)->displayName.find("`2") != string::npos) {
						size_t pos;
						while ((pos = static_cast<PlayerInfo*>(peer->data)->displayName.find("`2")) != string::npos) {
							static_cast<PlayerInfo*>(peer->data)->displayName.replace(pos, 2, "");
						}
						Player::OnNameChanged(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName);
					}
				}
			} catch (std::exception& e) {
				std::cout << "joinworld error" << std::endl;
				std::cout << e.what() << std::endl;
				return;
			}
		}
		sendWorld(peer, &info);
		SendInventory(peer, static_cast<PlayerInfo*>(peer->data)->inventory);
		if (x2 != 0 && y2 != 0) {
			x = x2;
			y = y2;
		}
		static_cast<PlayerInfo*>(peer->data)->x = x;
		static_cast<PlayerInfo*>(peer->data)->y = y;
		static_cast<PlayerInfo*>(peer->data)->lastx = 0; //antihack last pos
		static_cast<PlayerInfo*>(peer->data)->lasty = 0; // antihack last pos
		static_cast<PlayerInfo*>(peer->data)->disableanticheat = true;
		static_cast<PlayerInfo*>(peer->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 3;
		static_cast<PlayerInfo*>(peer->data)->checkx = x;
		static_cast<PlayerInfo*>(peer->data)->checky = y;
		int smstate = 0, is_invis = 0;
		if (static_cast<PlayerInfo*>(peer->data)->adminLevel >= 1) {
			if (info.width == 90 && info.height == 60 || info.width == 90 && info.height == 110) smstate = 0;
			else smstate = 1;
		}
		if (static_cast<PlayerInfo*>(peer->data)->isinv) is_invis = 1;
		/*spawn|avatar\nnetID|" + std::to_string(cId) + "\nuserID|" + std::to_string(cId) + "\ncolrect|0|0|20|30\nposXY|" + std::to_string(x) + "|" + std::to_string(y) + "\nname|" + static_cast<PlayerInfo*>(peer->data)->displayName + "``\ncountry|" + static_cast<PlayerInfo*>(peer->data)->country + "\ninvis|2\nmstate|0\nsmstate|" + to_string(smstate) + "\nonlineID|\ntype|local*/
		auto p = packetEnd(appendString(appendString(createPacket(), "OnSpawn"), "spawn|avatar\nnetID|" + std::to_string(cId) + "\nuserID|" + std::to_string(cId) + "\ncolrect|0|0|20|30\nposXY|" + std::to_string(x) + "|" + std::to_string(y) + "\nname|" + static_cast<PlayerInfo*>(peer->data)->displayName + "``\ncountry|" + static_cast<PlayerInfo*>(peer->data)->country + "\ninvis|" + to_string(is_invis) + "\nmstate|0\nsmstate|" + to_string(smstate) + "\nonlineID|\ntype|local\n"));
		const auto packet = enet_packet_create(p.data, p.len, ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(peer, 0, packet);
		delete p.data;
		static_cast<PlayerInfo*>(peer->data)->netID = cId;
		onPeerConnect(peer);
		cId++;
		if (!static_cast<PlayerInfo*>(peer->data)->isinv) {
			for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT") continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) continue;
					Player::OnConsoleMessage(currentPeer, "`5<`w" + static_cast<PlayerInfo*>(peer->data)->displayName + " `5dunyaya giris yapti, bu dunyada toplam `w" + std::to_string(getPlayersCountInWorld(static_cast<PlayerInfo*>(peer->data)->currentWorld) - 1) + "`` `5oyuncu var>```w");
					Player::OnTalkBubble(currentPeer, static_cast<PlayerInfo*>(peer->data)->netID, "`5<`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`` `5dunyaya giris yapti, bu dunyada toplam `w" + std::to_string(getPlayersCountInWorld(static_cast<PlayerInfo*>(peer->data)->currentWorld) - 1) + "`` `5oyuncu var>```w", 0, true);
					Player::PlayAudio(currentPeer, "audio/door_open.wav", 0);
				}
			}
		}
		if (ValentineEvent) {
			gamepacket_t p;
			p.Insert("OnProgressUISet");
			p.Insert(1);
			p.Insert(3402);
			p.Insert(static_cast<PlayerInfo*>(peer->data)->bootybreaken);
			p.Insert(100);
			p.Insert("");
			p.Insert(1);
			p.CreatePacket(peer);
		}
		static_cast<PlayerInfo*>(peer->data)->checkteleportspeedflyhacks = true;
	} catch(const std::out_of_range& e) {
		std::cout << "joinworld2 error" << std::endl;
		std::cout << e.what() << std::endl;
	} 
}



inline void handle_world(ENetPeer* peer, string act, bool sync = false, bool door = false, string destId = "", bool animations = true, int x = 0, int y = 0) {
	try {
		if (!static_cast<PlayerInfo*>(peer->data)->HasLogged && static_cast<PlayerInfo*>(peer->data)->haveGrowId) return;
		if (static_cast<PlayerInfo*>(peer->data)->lastJoinReq + 3000 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()) {
			static_cast<PlayerInfo*>(peer->data)->lastJoinReq = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
		}
		else {
			Player::OnConsoleMessage(peer, "`4EYVAH: `oAyni anda cok fazla kisi sunucuya giris yapiyor. Lutfen `5CANCEL`` tusuna basin ve birkac saniye sonra tekrar deneyin.`");
			enet_peer_disconnect_later(peer, 0);
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->NeedVerify) {
			Player::OnDialogRequest(peer, "text_scaling_string|Dirttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt|\nset_default_color|`o\nadd_label_with_icon|big|`wHesabiniz Kilitlendi``|left|206|\nadd_spacer|small|\nadd_textbox|Baska bir cihazdan giris tespit edildi bu dogrulamayi tamamlamak icin e-mailinizi alttaki kutucuga yazin.!|left|\nadd_spacer|small|\nadd_text_input|email|Email||64|\nadd_textbox|Sunucuya kayit olurkendki e-mailinizi giriniz aksi taktirde hesabiniza giris mumkun olmayacaktir!|left|\nend_dialog|verifyaap|Iptal|Kilidi Kaldir|\n");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		toUpperCase(act);
		if (act.size() > 24) return;
		if (act.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != string::npos) {
			Player::OnConsoleMessage(peer, "Sorry, spaces and special characters are not allowed in world or door names.  Try again.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "EXIT" && static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT") {
			Player::OnConsoleMessage(peer, "Zaten EXIT'desin aklini sikiyim.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		else if (act == "EXIT") {
			Player::OnConsoleMessage(peer, "Nereye gitmek istiyorsun? (`w" + GetPlayerCountServer() + " `ooyuncu aktif.)");
			sendPlayerLeave(peer);
			static_cast<PlayerInfo*>(peer->data)->currentWorld = "EXIT";
			sendWorldOffers(peer);
			return;
		}
		if (act == "ANAL" || act == "ANUS" || act == "ARSE" || act == "KONTOL" || act == "MEMEK" || act == "ASS" || act == "BALLSACK" || act == "BALLS" || act == "BASTARD" || act == "BITCH" || act == "BIATCH" || act == "BLOODY" || act == "BLOWJOB" || act == "BOLLOCK" || act == "BOLLOK" || act == "BONER" || act == "BOOB" || act == "BUGGER" || act == "BUM" || act == "BUTT" || act == "BUTTPLUG" || act == "CLITORIS" || act == "COCK" || act == "COON" || act == "CRAP" || act == "CUNT" || act == "DAMN" || act == "DICK" || act == "DILDO" || act == "DYKE" || act == "FAG" || act == "FECK" || act == "FELLATE" || act == "FELLATIO" || act == "FELCHING" || act == "FUCK" || act == "FUDGEPACKER" || act == "FLANGE" || act == "GODDAMN" || act == "HOMO" || act == "JERK" || act == "JIZZ" || act == "KNOBEND" || act == "LABIA" || act == "LMAO" || act == "LMFAO" || act == "MUFF" || act == "NIGGER" || act == "NIGGA" || act == "OMG" || act == "PENIS" || act == "PISS" || act == "POOP" || act == "PRICK" || act == "PUBE" || act == "PUSSY" || act == "QUEER" || act == "SCROTUM" || act == "SEX" || act == "SHIT" || act == "SH1T" || act == "SLUT" || act == "SMEGMA" || act == "SPUNK" || act == "TIT" || act == "TOSSER" || act == "TURD" || act == "TWAT" || act == "VAGINA" || act == "WANK" || act == "WHORE" || act == "WTF" || act == "SEBIA" || act == "ADMIN" || act == "SETH" || act == "HAMUMU" || act == "GOD" || act == "SATAN" || act == "RTSOFT" || act == "HEROMAN" || act == "SYSTEM" || act == "MIKEHOMMEL" || act == "SKIDS" || act == "MODERATOR" || act == "GODS" || act == "THEGODS" || act == "ALMANTAS") {
			Player::OnConsoleMessage(peer, "`4Bu dunyaya giris yapamazsiniz!?`` Baska bir world ismi deneyin?");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "GAME1" && game1status == true)
		{
			Player::OnConsoleMessage(peer, "`oMiniGame Zaten Basladi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "GAME1BACKUP")
		{
			Player::OnConsoleMessage(peer, "`oErisim Engellendi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		else if (act == "CASINO" && static_cast<PlayerInfo*>(peer->data)->canWalkInBlocks == true)
		{
			Player::OnConsoleMessage(peer, "Casino'ya hayalet olarak giremezsiniz");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "CROWS") {
			Player::OnConsoleMessage(peer, "Erisim engellendi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "GROWGANOTH")
		{
			Player::OnConsoleMessage(peer, "`4Growganoth'a `oHosgeldiniz, bu etkinlik 20 Kasim tarihinde sona erecektir. Tadini cikarmayi unutmayin!");
		}
		if (act == "LEYLABFG")
		{
			Player::OnConsoleMessage(peer, "`5Bu dunyaya girmeye bosuna calisma sadece Leyla girebilir.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "")
		{
			Player::OnConsoleMessage(peer, "`4Bu dunyaya giris yapamazsin burasi tamamen bos.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (!static_cast<PlayerInfo*>(peer->data)->haveGrowId) act = "BASLA";
		if (static_cast<PlayerInfo*>(peer->data)->isCursed) act = "CEHENNEM";
		if (static_cast<PlayerInfo*>(peer->data)->isHospitalized) act = "BASLA";
		if (static_cast<PlayerInfo*>(peer->data)->isjailed) act = "BASLA";
		if (static_cast<PlayerInfo*>(peer->data)->currentWorld != "EXIT" && !door) {
			sendPlayerLeave(peer);
		}
		if (std::experimental::filesystem::exists("save/worldbans/_" + act + "/" + static_cast<PlayerInfo*>(peer->data)->rawName)) {
			Player::OnConsoleMessage(peer, "`4Olamaz! ``Bu dunyadan owner veya admin tarafindan banlandin! Daha sonra tekrar dene.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		WorldInfo info;
		WorldInfo* info2;
		if (door && act == "")
		{
			info = worldDB.get(static_cast<PlayerInfo*>(peer->data)->currentWorld);
			info2 = worldDB.get_pointer(static_cast<PlayerInfo*>(peer->data)->currentWorld);
		}
		else
		{
			info = worldDB.get(act);
			info2 = worldDB.get_pointer(act);
		}
		if (act == "" && !door) {
			joinWorld(info, peer, "BASLA", 0, 0);
			return;
		}
		if (info.name == "error") {
			Player::OnConsoleMessage(peer, "HATA OLUSTU!");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (getPlayersCountInWorldSave(info.name) > 35) {
			Player::OnConsoleMessage(peer, "`5Eyvah, `4" + info.name + "`` `5dunyasinda `435`` kisi var. Daha sonra tekrar deneyiniz.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (act == "LEGENDARYMOUNTAIN" && !static_cast<PlayerInfo*>(peer->data)->quest_active || act == "LEGENDARYMOUNTAIN" && static_cast<PlayerInfo*>(peer->data)->quest_step < 20) {
			if (!isMod(peer)) {
				Player::OnConsoleMessage(peer, "`oBu dunyaya girecek kadar efsanevi degilsin...");
				Player::OnFailedToEnterWorld(peer);
				return;
			}
		}
		/*if (act == "CSN" && static_cast<PlayerInfo*>(peer->data)->level < 10 || act == "CSN" && atoi(static_cast<PlayerInfo*>(peer->data)->player_age.c_str()) < 18) {
			if (!isMod(peer) && !static_cast<PlayerInfo*>(peer->data)->Subscriber) {
				Player::OnConsoleMessage(peer, "You must have level 10 and be at least 18 years old to enter this world! (Or be a mod/subscriber) Currently you have level " + to_string(static_cast<PlayerInfo*>(peer->data)->level) + " and you are " + static_cast<PlayerInfo*>(peer->data)->player_age + " years old, work harder son!");
				Player::OnFailedToEnterWorld(peer);
				Player::PlayAudio(peer, "audio/fr2.wav", 0);
				return;
			}
		}*/
		if (act == "GROWGANOTH" && !GrowganothEvent && !isDev(peer)) {
			Player::OnConsoleMessage(peer, "Growganoth Eventi Daha Baslamadi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (info.isNuked && !isMod(peer)) {
			Player::OnConsoleMessage(peer, "Dunyaya Erisim Engellendi.");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if (info.entrylevel > static_cast<PlayerInfo*>(peer->data)->level && info.owner != static_cast<PlayerInfo*>(peer->data)->rawName && !isMod(peer)) {
			Player::OnConsoleMessage(peer, " " + to_string(info.entrylevel) + " levelinden az olan oyuncular bu worlde giremez " + info.name + ".");
			Player::OnFailedToEnterWorld(peer);
			return;
		}
		if ((info.width == 90 && info.height == 60 && static_cast<PlayerInfo*>(peer->data)->canWalkInBlocks) || (info.height == 110 && static_cast<PlayerInfo*>(peer->data)->canWalkInBlocks)) {
			SendGhost(peer);
		}	
		if (door) {
			if (destId != "") {
				for (auto i = 0; i < info.width * info.height; i++) {
					if (getItemDef(info.items.at(i).foreground).blockType == BlockTypes::DOOR || getItemDef(info.items.at(i).foreground).blockType == BlockTypes::PORTAL) {
						if (info.items.at(i).currId == destId) {
							if (act == static_cast<PlayerInfo*>(peer->data)->currentWorld || act == "") {
								DoCancelTransitionAndTeleport(peer, (i % info.width), (i / info.width), false, animations);
							}
							else {
								joinWorld(info, peer, act, (i % info.width) * 32, (i / info.width) * 32);
							}
							return;
						}
					}
				}
			}
			for (auto s = 0; s < info.width * info.height; s++) {
				if (info.items.at(s).foreground == 6) {
					if (act == static_cast<PlayerInfo*>(peer->data)->currentWorld || act == "") {
						DoCancelTransitionAndTeleport(peer, (s % info.width), (s / info.width), false, animations);
					}
					else {
						joinWorld(info, peer, act, 0, 0);
					}
					return;
				}
			}
			return;
		}
		joinWorld(info, peer, act, x, y);
		if (sync) Player::PlayAudio(peer, "audio/choir.wav", 250);
	}
	catch (const std::out_of_range& e) {
		cout << "handle_world error" << endl;
		std::cout << e.what() << std::endl;
	}
}
inline void DoEnterDoor(ENetPeer* peer, WorldInfo* world, int x, int y, bool animations = true) {
	if (static_cast<PlayerInfo*>(peer->data)->Fishing) {
		static_cast<PlayerInfo*>(peer->data)->TriggerFish = false;
		static_cast<PlayerInfo*>(peer->data)->FishPosX = 0;
		static_cast<PlayerInfo*>(peer->data)->FishPosY = 0;
		static_cast<PlayerInfo*>(peer->data)->Fishing = false;
		send_state(peer);
		Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wSit perfectly when fishing!", 0, false);
		Player::OnSetPos(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y);
	}
	handle_world(peer, world->items.at(x + (y * world->width)).destWorld, false, true, world->items.at(x + (y * world->width)).destId, animations);
}

inline void sendTileUpdate(int x, int y, int tile, int causedBy, ENetPeer* peer, WorldInfo* world) {									
	PlayerInfo* pData = GetPeerData(peer);
	if (pData->trade) end_trade(peer);
	Player::Ping(peer); /*not sure*/
	if (world == NULL || pData->isIn == false || pData->currentWorld == "EXIT" || tile > itemDefs.size()) return;
	if (static_cast<PlayerInfo*>(peer->data)->isantibotquest) return;
	auto dicenr = 0;
	PlayerMoving data{};        
 	data.packetType = 0x3;
	data.characterState = 0x0;
	data.x = x;
	data.y = y;
	data.punchX = x;
	data.punchY = y;
	data.XSpeed = 0;
	data.YSpeed = 0;
	data.netID = causedBy;
	data.plantingTree = tile;
	int squaresign = x + (y * world->width);
	bool isTree = false, isLock = false, isSmallLock = false, isvip = false, isHeartMonitor = false, isMannequin = false, isScience = false, isMagplant = false, isgateway = false, iscontains = false, VendUpdate = false, isDoor = false;
	if (world == nullptr || x < 0 || y < 0 || world->name == "CEHENNEM" && pData->isCursed || x > world->width - 1 || y > world->height - 1 || tile > itemDefs.size()) return;
	if (tile != 18 && tile != 32 && tile != 6336) {
		auto contains = false;
		SearchInventoryItem(peer, tile, 1, contains);
		if (!contains) return;
	}
	if (world->name == "GAME1BACKUP") return;
	if (world->name == "GAME1")
	{
		if (game1status == true) return;
	}
	if (world->saved) world->saved = false;
	if (tile == 18) pData->totalpunch++;
	try {
		if (world->items.at(x + (y * world->width)).background == 3556 && pData->cloth_hand != 3066 && world->width == 90 && world->height == 60 && world->items.at(x + (y * world->width)).foreground == 0 || world->items.at(x + (y * world->width)).foreground == 9150 && pData->cloth_hand != 3066 && world->width == 90 && world->height == 60) {
			if (tile == 18 || getItemDef(tile).blockType == BlockTypes::BACKGROUND) return;
		}
		if (getItemDef(tile).blockType == BlockTypes::SEED && !world->items.at(x + (y * world->width)).fire) {
			if (world->items.at(x + (y * world->width)).foreground != 0) {
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SEED) {
					if (world->isPublic || isWorldAdmin(peer, world) || pData->rawName == world->owner || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (calcBanDuration(world->items.at(x + (y * world->width)).growtime) == 0) {
							Player::OnTalkBubble(peer, pData->netID, "Bu tohum zaten cok fazla buyuk.", 0, true);
							return;
						}
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).name == getItemDef(tile).name || getItemDef(world->items.at(x + (y * world->width)).foreground).rarity == 999 || getItemDef(tile).rarity == 999) {
							Player::OnTalkBubble(peer, pData->netID, "Gorunuyor ki, `w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`` ve `w" + getItemDef(tile).name + "`` ile yeni seed olusturamazsiniz.", 0, true);
							return;
						}
						if (!world->items.at(x + (y * world->width)).spliced) {
							int target_seed = 0, result_seed = 0;
							bool reverse = false;
							ifstream infile("recipes.txt");
							for (string line; getline(infile, line);) {
								if (line.length() > 3 && line.at(0) != '/' && line.at(1) != '/') {
									size_t pos;
									while ((pos = line.find("\r")) != string::npos) {
										line.replace(pos, 1, "");
									}
									auto ex = explode("|", line);
									if (ex.at(1) == to_string(tile) && ex.at(2) == to_string(world->items.at(x + (y * world->width)).foreground)) {
										result_seed = atoi(ex.at(0).c_str()) + 1;
										target_seed = atoi(ex.at(2).c_str());
										break;
									} else if (ex.at(1) == to_string(world->items.at(x + (y * world->width)).foreground) && ex.at(2) == to_string(tile)) {
										result_seed = atoi(ex.at(0).c_str()) + 1;
										target_seed = atoi(ex.at(2).c_str());
										reverse = true;
										break;
									}
								}
							} 
							infile.close();
							if (reverse && tile == target_seed && target_seed != 0 && result_seed != 0 || world->items.at(x + (y * world->width)).foreground == target_seed && target_seed != 0 && result_seed != 0) {
								splice_seed(peer, pData, world, tile, x, y, result_seed);
								return;
							}
							auto targetvalue = getItemDef(world->items.at(x + (y * world->width)).foreground).rarity + getItemDef(tile).rarity;
							if (pData->level >= 15) targetvalue += 1;
							for (auto i = 0; i < 10016; i++) {
								if (getItemDef(i).rarity == targetvalue && !isSeed(i)) {
									world->items.at(x + (y * world->width)).spliced = true;
									Player::OnTalkBubble(peer, pData->netID, "`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`` and `w" + getItemDef(tile).name + "`` have been spliced to make a `$" + getItemDef(i + 1).name + "``!", 0, true);
									Player::PlayAudio(peer, "audio/success.wav", 0);
									world->items.at(x + (y * world->width)).foreground = i + 1;
									int growTimeSeed = getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
									growTimeSeed += 30 * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
									world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + growTimeSeed);
									if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity == 999) {
										world->items.at(x + (y * world->width)).fruitcount = (rand() % 1) + 1;
									} else {
										world->items.at(x + (y * world->width)).fruitcount = (rand() % 3) + 1;
									}
									if (getItemDef(world->items.at(x + (y * world->width)).foreground - 1).blockType == BlockTypes::CLOTHING) world->items.at(x + (y * world->width)).fruitcount = 1;
									UpdateTreeVisuals(peer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).fruitcount, 0, true, 0);
									spray_tree(peer, world, x, y, 18, true);
									RemoveInventoryItem(tile, 1, peer, true);
									break;
								} else if (i >= 10015) {
									Player::OnTalkBubble(peer, pData->netID, "Hmm, it looks like `w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`` and `w" + getItemDef(tile).name + "`` can't be spliced.", 0, true);
									break;
								}
							}
						} else {
							Player::OnTalkBubble(peer, pData->netID, "It would be too dangerous to try to mix three seeds.", 0, true);
						}
					}
					return;
				}
			} else {
				isTree = true;
			}
		}
		switch (tile) {
		case 18: /*punch*/
			{
				if (world->items.at(x + (y * world->width)).foreground == 5638) {
					bool canuseremote = true;
					if (canuseremote) {
						if (pData->inventory.items.size() == pData->currentInventorySize) {
							//Player::OnTalkBubble(peer, pData->netID, "`wInventory is full!", 0, true);
						} else {
							bool iscontainss = false;
							SearchInventoryItem(peer, 5640, 1, iscontainss);
							if (!iscontainss) {
								if (isWorldOwner(peer, world) && world->items.at(x + (y * world->width)).mid != 0 && world->items.at(x + (y * world->width)).mc != 0 || world->items.at(x + (y * world->width)).rm && world->items.at(x + (y * world->width)).mid != 0 && world->items.at(x + (y * world->width)).mc != 0) {
									bool Farmable = false;
									if (world->items.at(x + (y * world->width)).mid == 7382 || world->items.at(x + (y * world->width)).mid == 4762 || world->items.at(x + (y * world->width)).mid == 5140 || world->items.at(x + (y * world->width)).mid == 5138 || world->items.at(x + (y * world->width)).mid == 5136 || world->items.at(x + (y * world->width)).mid == 5154 || world->items.at(x + (y * world->width)).mid == 340 || world->items.at(x + (y * world->width)).mid == 954 || world->items.at(x + (y * world->width)).mid == 5666) Farmable = true;
									if (world->items.at(x + (y * world->width)).foreground == 5638 && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::FOREGROUND || world->items.at(x + (y * world->width)).foreground == 5638 && Farmable || world->items.at(x + (y * world->width)).foreground == 5638 && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::BACKGROUND || world->items.at(x + (y * world->width)).foreground == 5638 && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::GROUND_BLOCK) {
										if (getItemDef(world->items.at(x + (y * world->width)).mid).properties & Property_AutoPickup) {
											/*...*/
										} else {
											Player::OnTalkBubble(peer, pData->netID, "`wYou received a MAGPLANT 5000 Remote.", 0, true);
											bool success = true;
											SaveItemMoreTimes(5640, 1, peer, success);
											pData->magplantitemid = world->items.at(x + (y * world->width)).mid;
											pData->magplantx = x;
											pData->magplanty = y;
										}
									}
								}
							} else {
								pData->magplantitemid = world->items.at(x + (y * world->width)).mid;
								pData->magplantx = x;
								pData->magplanty = y;
								//Player::OnTalkBubble(peer, pData->netID, "`wYou received a MAGPLANT 5000 Remote.", 0, true);
							}
						}
					}
				}
				if (pData->cloth_hand == 3066) {
					if (world->items.at(x + (y * world->width)).fire) {
						int chanceofgems = 1;
						if (pData->firefighterlevel >= 4) chanceofgems = 2;
						if (pData->firefighterlevel >= 6) chanceofgems = 3;
						if (pData->firefighterlevel >= 8) chanceofgems = 4;
						if (pData->firefighterlevel >= 10) chanceofgems = 5;
						if (pData->firefighterlevel >= 2 && rand() % 100 <= chanceofgems) {
							auto Gems = (rand() % 25) + 1;
							auto currentGems = 0;
							ifstream ifs("save/gemdb/_" + pData->rawName + ".txt");
							ifs >> currentGems;
							ifs.close();
							currentGems += Gems;
							ofstream ofs("save/gemdb/_" + pData->rawName + ".txt");
							ofs << currentGems;
							ofs.close();
							Player::OnConsoleMessage(peer, "Fire King bonus obtained " + to_string(Gems) + " gems");
							Player::OnSetBux(peer, currentGems, 0);
						} else if (pData->firefighterlevel >= 3 && rand() % 100 <= 1) {
							bool success = true;
							SaveItemMoreTimes(4762, 1, peer, success, "");
							Player::OnConsoleMessage(peer, "Obtained Amethyst Block");
						} else if (pData->firefighterlevel >= 4 && rand() % 100 <= 2) {
							bool success = true;
							SaveItemMoreTimes(7156, 1, peer, success, "");
							Player::OnConsoleMessage(peer, "Obtained Fallen Pillar");
						} else if (pData->firefighterlevel >= 7 && rand() % 100 <= 1) {
							bool success = true;
							SaveItemMoreTimes(5138, 1, peer, success, "");
							Player::OnConsoleMessage(peer, "Obtained Diamond Stone");
						} else if (pData->firefighterlevel == 10 && rand() % 300 <= 1 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::BACKGROUND || pData->firefighterlevel == 10 && rand() % 300 <= 1 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SFX_FOREGROUND || pData->firefighterlevel == 10 && rand() % 300 <= 1 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::FOREGROUND) {
							bool success = true;
							SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success, "");
							Player::OnConsoleMessage(peer, "Obtained " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "");
						}
						int targetfirelevel = 1500;
						if (pData->firefighterlevel > 0) targetfirelevel = targetfirelevel * pData->firefighterlevel;
						if (pData->firefighterlevel == 0) targetfirelevel = 750;
						if (pData->firefighterxp >= targetfirelevel && pData->firefighterlevel < 10) {
							pData->firefighterlevel++;
							pData->firefighterxp = 0;
							SyncPlayerRoles(peer, pData->firefighterlevel, "firefighter");
						} else {
							pData->firefighterxp++;
						}
						world->items.at(x + (y * world->width)).fire = false;
						UpdateBlockState(peer, x, y, true, world);
						for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
							if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
							if (isHere(peer, net_peer)) {
								Player::OnParticleEffect(net_peer, 149, x * 32, y * 32, 0);
							}
						}
					 }
					return;
				}
				if (pData->cloth_hand == 4996 && !world->items.at(x + (y * world->width)).fire) {
					if (isSeed(world->items.at(x + (y * world->width)).foreground) || world->items.at(x + (y * world->width)).water || world->items.at(x + (y * world->width)).foreground == 6952 || world->items.at(x + (y * world->width)).foreground == 6954 || world->items.at(x + (y * world->width)).foreground == 5638 || world->items.at(x + (y * world->width)).foreground == 6946 || world->items.at(x + (y * world->width)).foreground == 6948 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::VENDING || world->items.at(x + (y * world->width)).foreground == 1420 || world->items.at(x + (y * world->width)).foreground == 6214 || world->items.at(x + (y * world->width)).foreground == 1006 || world->items.at(x + (y * world->width)).foreground == 1420 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DONATION || world->items.at(x + (y * world->width)).foreground == 3528  || world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 6864 || world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0 || world->items.at(x + (y * world->width)).foreground == 6 || world->items.at(x + (y * world->width)).foreground == 8 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DISPLAY) {
						if (world->items.at(x + (y * world->width)).background != 6864) Player::OnTalkBubble(peer, pData->netID, "`wCan't burn that!", 0, true);
						return;
					}
					world->items.at(x + (y * world->width)).fire = true;
					for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
						if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
						if (isHere(peer, net_peer)) {
							Player::OnParticleEffect(net_peer, 150, x * 32 + 16, y * 32 + 16, 0);
						}
					}
					UpdateVisualsForBlock(peer, true, x, y, world);
					return;
				}
				if (world->items.at(x + (y * world->width)).fire) return;
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::PROVIDER) {
					if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner) || isDev(peer)) {
						if (calcBanDuration(world->items.at(x + (y * world->width)).growtime) == 0) {
							if (pData->quest_active && pData->lastquest == "honor" && pData->quest_step == 13 && pData->quest_progress < 1000) {
								pData->quest_progress++;
								if (pData->quest_progress >= 1000) {
									pData->quest_progress = 1000;
									Player::OnTalkBubble(peer, pData->netID, "`9Legendary Quest step complete! I'm off to see a Wizard!", 0, false);
								}
							}
							int chanceofdouble = 1;
							int weedmagic = 1;
							if (pData->providerlevel >= 4) chanceofdouble = 2;
							if (pData->providerlevel >= 6) chanceofdouble = 3;
							if (pData->providerlevel >= 8) chanceofdouble = 4;
							if (pData->providerlevel >= 10) chanceofdouble = 5;
							if (pData->providerlevel >= 2 && rand() % 100 <= chanceofdouble) {
								weedmagic = 2;
								Player::OnConsoleMessage(peer, "Weed Magic bonus doubled item");
							}
							else if (pData->providerlevel >= 4 && rand() % 100 <= 1) {
								bool success = true;
								SaveItemMoreTimes(5136, 1, peer, success, "");
								Player::OnConsoleMessage(peer, "Obtained Smaraged Block");
							}
							else if (pData->providerlevel >= 7 && rand() % 100 <= 1) {
								bool success = true;
								SaveItemMoreTimes(2410, 1, peer, success, "");
								Player::OnConsoleMessage(peer, "Obtained Emerald Shard");
							}
							else if (pData->providerlevel == 10 && rand() % 300 <= 1) {
								Player::OnConsoleMessage(peer, "The " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " has dropped himself");
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), world->items.at(x + (y * world->width)).foreground, 1, 0);
							}
							int targetproviderlevel = 1300;
							if (pData->providerlevel > 0) targetproviderlevel = targetproviderlevel * pData->providerlevel;
							if (pData->providerlevel == 0) targetproviderlevel = 600;
							if (pData->providerxp >= targetproviderlevel && pData->providerlevel < 10) {
								pData->providerlevel++;
								pData->providerxp = 0;
								SyncPlayerRoles(peer, pData->providerlevel, "provider");
							} else {
								pData->providerxp++;
							}

							if (world->items.at(x + (y * world->width)).foreground == 872) {						
								if (rand() % 100 <= 25) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 874, 2 * weedmagic, 0);
								else DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 874, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 866) {
								if (rand() % 100 <= 25) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 868, 2 * weedmagic, 0);
								else DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 868, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 3888) {
								if (pData->cloth_hand == 3914) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 3890, rand() % 5 + 1 * weedmagic, 0);
								else DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 3890, rand() % 2 + 1 * weedmagic, 0);
							}
							if (world->items.at(x + (y * world->width)).foreground == 8196) {
								int ItemID = rand() % maxItems;
								while (getItemDef(ItemID).blockType == BlockTypes::CLOTHING || getItemDef(ItemID).properties & Property_Untradable || getItemDef(ItemID).name.find("Ancestral") != string::npos  || getItemDef(ItemID).name.find("null_item") != string::npos || isSeed(ItemID) || ItemID == 1458) {
									ItemID = rand() % maxItems;
								}
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1, 0);
								for (ENetPeer* peer2 = server->peers; peer2 < &server->peers[server->peerCount]; ++peer2) {
									if (peer2->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, peer2)) {
										Player::OnParticleEffect(peer2, 182, x * 32, y * 32, 0);
									}
								}
							}
							if (world->items.at(x + (y * world->width)).foreground == 10450) {
								int ItemID = rand() % maxItems;
								while (getItemDef(ItemID).blockType != BlockTypes::CLOTHING || isSeed(ItemID) || ItemID == 18 || ItemID == 32 || ItemID == 6336 || ItemID == 9510 || ItemID == 9506) {
									ItemID = rand() % maxItems;
								}
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1, 0);
								for (ENetPeer* peer2 = server->peers; peer2 < &server->peers[server->peerCount]; ++peer2) {
									if (peer2->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, peer2)) {
										Player::OnParticleEffect(peer2, 182, x * 32, y * 32, 0);
									}
								}
							}
							if (world->items.at(x + (y * world->width)).foreground == 928) {
								vector<int> Dailyb{ 914, 916, 918, 920, 924 };
								const int Index = rand() % Dailyb.size();
								const auto ItemID = Dailyb[Index];
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 1044) {
								if (rand() % 100 <= 25) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 868, 2 * weedmagic, 0);
								else DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 868, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 2798) {
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 822, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 1008) {
								SendFarmableDrop(peer, 500, x, y, world);
							}

							if (world->items.at(x + (y * world->width)).foreground == 1636) {
								vector<int> Dailyb{ 728, 360, 308, 306, 2966, 1646, 3170, 1644, 1642, 3524, 1640, 1638, 2582, 3198, 8838, 6794, 10110 };
								const int Index = rand() % Dailyb.size();
								const auto ItemID = Dailyb[Index];
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 3044) {
								vector<int> Dailyb{ 2914, 3012, 3014, 3016, 3018, 5528, 5526 };
								const int Index = rand() % Dailyb.size();
								const auto ItemID = Dailyb[Index];
								if (rand() % 100 <= 25) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 2 * weedmagic, 0);
								else DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 6212) {
								vector<int> Dailyb{ 1258, 1260, 1262, 1264, 1266, 1268, 1270, 4308, 4310, 4312, 4314, 4316, 4318 };
								const int Index = rand() % Dailyb.size();
								const auto ItemID = Dailyb[Index];
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 6414) {
								vector<int> Dailyb{ 6520, 6538, 6522, 6528, 6540, 6518, 6530, 6524, 6536, 6534, 6532, 6526, 6416 };
								const int Index = rand() % Dailyb.size();
								const auto ItemID = Dailyb[Index];
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ItemID, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 5116) {
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 5114, 1 * weedmagic, 0);
							}

							if (world->items.at(x + (y * world->width)).foreground == 1632) {
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 1634, 1 * weedmagic, 0);
							}





							int buvoid = world->items.at(x + (y * world->width)).foreground;
							world->items.at(x + (y * world->width)).foreground = 0;
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y;
							data3.punchX = x;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 0;
							ENetPeer* currentPeer;
							for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer)) {
									auto raw = packPlayerMoving(&data3);
									raw[2] = dicenr;
									raw[3] = dicenr;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							world->items.at(x + (y * world->width)).foreground = buvoid;
							PlayerMoving data4{};
							data4.packetType = 0x3;
							data4.characterState = 0x0;
							data4.x = x;
							data4.y = y;
							data4.punchX = x;
							data4.punchY = y;
							data4.XSpeed = 0;
							data4.YSpeed = 0;
							data4.netID = -1;
							data4.plantingTree = buvoid;
							for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer)) {
									auto raw = packPlayerMoving(&data4);
									raw[2] = dicenr;
									raw[3] = dicenr;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + getItemDef(world->items.at(x + (y * world->width)).foreground).growTime);
						}
					}
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SHELF) {
					if (std::experimental::filesystem::exists("save/dshelf/" + ((PlayerInfo*)(peer->data))->currentWorld + "/X" + std::to_string(squaresign) + ".txt"))
					{
						int item1 = 0;
						int item2 = 0;
						int item3 = 0;
						int item4 = 0;
						ifstream fd("save/dshelf/" + ((PlayerInfo*)(peer->data))->currentWorld + "/X" + std::to_string(squaresign) + ".txt");
						fd >> item1;
						fd >> item2;
						fd >> item3;
						fd >> item4;
						fd.close();
						sendDShelf(peer, world->items.at(x + (y * world->width)).foreground, x, y, item1, item2, item3, item4);
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 1420 || world->items.at(x + (y * world->width)).foreground == 6214)
				{
					if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner))
					{
						auto seedexist = std::experimental::filesystem::exists("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						if (seedexist)
						{
							json j;
							ifstream fs("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							fs >> j;
							fs.close();
							bool found = false, success = false;
							if (j["clothHead"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothHead"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothHead"] = "0";
							}
							else if (j["clothHair"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothHair"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothHair"] = "0";
							}
							else if (j["clothMask"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothMask"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothMask"] = "0";
							}
							else if (j["clothNeck"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothNeck"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothNeck"] = "0";
							}
							else if (j["clothBack"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothBack"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothBack"] = "0";
							}
							else if (j["clothShirt"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothShirt"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothShirt"] = "0";
							}
							else if (j["clothPants"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothPants"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothPants"] = "0";
							}
							else if (j["clothFeet"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothFeet"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothFeet"] = "0";
							}
							else if (j["clothHand"].get<string>() != "0")
							{
								SaveItemMoreTimes(atoi(j["clothHand"].get<string>().c_str()), 1, peer, success, pData->rawName + " from Mannequin");
								found = true;
								j["clothHand"] = "0";
							}

							if (found)
							{
								updateMannequin(peer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).sign,
									atoi(j["clothHair"].get<string>().c_str()), atoi(j["clothHead"].get<string>().c_str()),
									atoi(j["clothMask"].get<string>().c_str()), atoi(j["clothHand"].get<string>().c_str()), atoi(j["clothNeck"].get<string>().c_str()),
									atoi(j["clothShirt"].get<string>().c_str()), atoi(j["clothPants"].get<string>().c_str()), atoi(j["clothFeet"].get<string>().c_str()),
									atoi(j["clothBack"].get<string>().c_str()), true, 0);

								ofstream of("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								of << j;
								of.close();
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 758) sendRoulete(peer);
				bool hassmallock = false;
				for (int i = 0; i < world->width * world->height; i++) {
					if (world->items.at(i).foreground == 202 || world->items.at(i).foreground == 204 || world->items.at(i).foreground == 206 || world->items.at(i).foreground == 4994) {
						hassmallock = true;
						break;
					}
				}
				if (hassmallock && !isDev(peer) || world->owner != "" && !isWorldOwner(peer, world)) {
					if (!isDev(peer)) {
						if (!restricted_area(peer, world, x, y)) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								if (pData->rawName != whoslock) {
									try {
										ifstream read_player("save/players/_" + whoslock + ".json");
										if (!read_player.is_open()) {
											return;
										}		
										json j;
										read_player >> j;
										read_player.close();
										string nickname = j["nick"];
										int adminLevel = j["adminLevel"];
										if (nickname == "") {
											nickname = role_prefix.at(adminLevel) + whoslock;
										} 
										if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
										else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4YETKI YOK`w)", 0, true);
										Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
									} catch (std::exception& e) {
										std::cout << e.what() << std::endl;
										return;
									}
									return;
								}
							}
						}
						else if (isWorldAdmin(peer, world)) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`2YETKI SAGLANDI`w)", 0, true);
									Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
								return;
							}
						}
						else if (world->isPublic)
						{
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK)
							{
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
									Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
								return;
							}
						} else if (world->isEvent) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK)
							{
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								if (pData->rawName != whoslock) {
									try {
										ifstream read_player("save/players/_" + whoslock + ".json");
										if (!read_player.is_open()) {
											return;
										}		
										json j;
										read_player >> j;
										read_player.close();
										string nickname = j["nick"];
										int adminLevel = j["adminLevel"];
										if (nickname == "") {
											nickname = role_prefix.at(adminLevel) + whoslock;
										} 
										if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
										else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4YETKI YOK`w)", 0, true);
										Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
									} catch (std::exception& e) {
										std::cout << e.what() << std::endl;
										return;
									}
									return;
								}
							}
							else if (world->items.at(x + (y * world->width)).foreground != world->publicBlock && causedBy != -1) {
								if (world->items.at(x + (y * world->width)).foreground != 0) Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								else if (world->items.at(x + (y * world->width)).background != 0) Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								return;
							}
						} else {
							Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
							return;
						}
					}
					if (tile == 18 && isDev(peer))
					{
						if (isWorldAdmin(peer, world) && !isWorldOwner(peer, world))
						{
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK)
							{
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`2YETKI SAGLANDI`w)", 0, true);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
							}
						}
						else if (world->isPublic && !isWorldOwner(peer, world))
						{
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK)
							{
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`HERKESE ACIK`w)", 0, true);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
							}
						}
						else if (world->isEvent && !isWorldOwner(peer, world))
						{
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK)
							{
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								if (pData->rawName != whoslock) {
									try {
										ifstream read_player("save/players/_" + whoslock + ".json");
										if (!read_player.is_open()) {
											return;
										}		
										json j;
										read_player >> j;
										read_player.close();
										string nickname = j["nick"];
										int adminLevel = j["adminLevel"];
										if (nickname == "") {
											nickname = role_prefix.at(adminLevel) + whoslock;
										} 
										if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9Open to Public`w)", 0, true);
										else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4No Access`w)", 0, true);
									} catch (std::exception& e) {
										std::cout << e.what() << std::endl;
										return;
									}
								}
							}
						}
					}
				} if (!isDev(peer)) {
					if (world->items.at(x + (y * world->width)).foreground == 6 || world->items.at(x + (y * world->width)).foreground == 9380 || world->items.at(x + (y * world->width)).foreground == 8440 && pData->cloth_hand != 98|| world->items.at(x + (y * world->width)).foreground == 9146 && pData->cloth_hand != 98|| world->items.at(x + (y * world->width)).foreground == 8532 && pData->cloth_hand != 98|| world->items.at(x + (y * world->width)).foreground == 8530 && pData->cloth_hand != 98) {
						Player::OnTalkBubble(peer, pData->netID, "`wIt's too strong to break.", 0, true);
						return;
					} if (world->items.at(x + (y * world->width)).foreground == 8 || world->items.at(x + (y * world->width)).foreground == 7372 || world->items.at(x + (y * world->width)).foreground == 3760) {
						if (pData->cloth_hand == 8452) {
						} else {
							Player::OnTalkBubble(peer, pData->netID, "`wIt's too strong to break.", 0, true);
							return;
						}
					} if (tile == 9444) {
						if (pData->cloth_hand == 2952 || pData->cloth_hand == 9430 || pData->cloth_hand == 9448 || pData->cloth_hand == 9452 || pData->cloth_hand == 2592) {
						} else {
							Player::OnTalkBubble(peer, pData->netID, "`5This stone is too strong!", 0, true);
							return;
						}
					}
					if (tile == 6 || tile == 3760 || tile == 1000 || tile == 7372 || tile == 1770 || tile == 1832 || tile == 4720) {
						Player::OnTalkBubble(peer, pData->netID, "`wIt's too heavy to place.", 0, true);
						return;
					}
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::WEATHER && isWorldOwner(peer, world) || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::WEATHER && isWorldAdmin(peer, world) || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::WEATHER && isDev(peer)) SendWeather(peer, tile, world, x, y);
				if (world->items.at(x + (y * world->width)).foreground == 3694) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 28;
							sendHeatwave(peer, x, y, world->items.at(x + (y * world->width)).vid, world->items.at(x + (y * world->width)).vprice, world->items.at(x + (y * world->width)).vcount);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 28);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 3832) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 29;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 29);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 10286) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 51;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 51);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 8836) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 48;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 48);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 8896) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 47;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 47);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 4892) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 33;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 33);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 6854) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 42;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 42);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 5654) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 36;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 36);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 5716) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 37;
							sendStuffweather(peer, x, y, world->items.at(x + (y * world->width)).intdata, world->items.at(x + (y * world->width)).mc, world->items.at(x + (y * world->width)).rm, world->items.at(x + (y * world->width)).opened);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 37);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 5000) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).activated) {
							world->items.at(x + (y * world->width)).activated = false;
							world->weather = 0;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 0);
								}
							}
						}
						else {
							world->items.at(x + (y * world->width)).activated = true;
							world->weather = 34;
							sendBackground(peer, x, y, world->items.at(x + (y * world->width)).intdata);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									Player::OnSetCurrentWeather(currentPeer, 34);
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 2946) {
					if (world->isPublic || isWorldAdmin(peer, world)) {
						Player::OnTalkBubble(peer, pData->netID, "Buranin sahibi " + world->owner + "", 0, true);
						return;
					}
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::RANDOM_BLOCK) {
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (isHere(peer, currentPeer)) {
							send_dice(currentPeer, rand() % 5 + 1, x, y);
						}
					}
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::TOGGLE_FOREGROUND || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::CHEMICAL_COMBINER || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::CHEST) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::CHEMICAL_COMBINER) {
							if (world->items.at(x + (y * world->width)).activated) {
								SendCombiner(peer, world, x, y);
								UpdateBlockState(peer, x, y, true, world);
								world->items.at(x + (y * world->width)).activated = false;
							} else {
								UpdateBlockState(peer, x, y, true, world);
								world->items.at(x + (y * world->width)).activated = true;
							}
						} else {
							if (world->items.at(x + (y * world->width)).activated) {
								UpdateBlockState(peer, x, y, true, world);
								world->items.at(x + (y * world->width)).activated = false;
								if (world->items.at(x + (y * world->width)).foreground == 4992)
								{
									for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
										if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
										if (isHere(peer, currentPeer)) {
											static_cast<PlayerInfo*>(currentPeer->data)->worldContainsActivatedAntiGravity = false;
										}
									}
								}
							}
							else {
								UpdateBlockState(peer, x, y, true, world);
								world->items.at(x + (y * world->width)).activated = true;
								if (world->items.at(x + (y * world->width)).foreground == 4992)
								{
									for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
										if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
										if (isHere(peer, currentPeer)) {
											static_cast<PlayerInfo*>(currentPeer->data)->worldContainsActivatedAntiGravity = true;
										}
									}
								}
							}
						}
					}
				}
				if (isSeed(world->items.at(x + (y * world->width)).foreground) && calcBanDuration(world->items.at(x + (y * world->width)).growtime) == 0 && pData->cloth_hand != 3066) {
					if (world->isPublic || isWorldAdmin(peer, world) || isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->items.at(x + (y * world->width)).foreground == 1791) { /*lwizard*/
							world->items.at(x + (y * world->width)).foreground = 1790;
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y;
							data3.punchX = x;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 1790;
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer)) {
									auto raw = packPlayerMoving(&data3);
									raw[2] = dicenr;
									raw[3] = dicenr;
									Player::OnParticleEffect(peer, 48, x * 32, y * 32, 0);
									SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							return;
						}
						auto chance = (rand() % 100) + 1;
						int numb = world->items.at(x + (y * world->width)).fruitcount;
						DropGem(peer, x, y, world);
						string drop_message = getItemDef(world->items.at(x + (y * world->width)).foreground).name;
						int drop_id = world->items.at(x + (y * world->width)).foreground;
						if (world->items.at(x + (y * world->width)).foreground == 1259 || world->items.at(x + (y * world->width)).foreground == 1261 || world->items.at(x + (y * world->width)).foreground == 1263 || world->items.at(x + (y * world->width)).foreground == 1265 || world->items.at(x + (y * world->width)).foreground == 1267 || world->items.at(x + (y * world->width)).foreground == 1269 || world->items.at(x + (y * world->width)).foreground == 1271 || world->items.at(x + (y * world->width)).foreground == 4309 || world->items.at(x + (y * world->width)).foreground == 4311 || world->items.at(x + (y * world->width)).foreground == 4313 || world->items.at(x + (y * world->width)).foreground == 4315 || world->items.at(x + (y * world->width)).foreground == 4317 || world->items.at(x + (y * world->width)).foreground == 4319) {
							drop_message = "Surgical Tool Tree";
							vector<int> random_surg{ 1258, 1260, 1262, 1264, 1266, 1268, 1270, 4308, 4310, 4312, 4314, 4316, 4318 };
							drop_id = random_surg.at(rand() % random_surg.size()) + 1;
						} if (chance < 15 && getItemDef(world->items.at(x + (y * world->width)).foreground).rarity != 999) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground - 1).blockType != BlockTypes::CLOTHING) {
								Player::OnTalkBubble(peer, pData->netID, "A " + drop_message + " seed falls out!", 0, false);
								DropItem(world, peer, -1, x * 32 - (rand() % 8), y * 32 + (rand() % 8), world->items.at(x + (y * world->width)).foreground, 1, 0);
							}
						} if (pData->cloth_hand == 6840) {
							auto chance1 = (rand() % 100) + 1;
							if (chance1 <= 30) {
								numb += 1;
								Player::OnParticleEffect(peer, 49, data.punchX * 32, data.punchY * 32, 0);
							}
						}
						int chanceofbuff = 1;
						if (pData->level >= 10) chanceofbuff = 1;
						if (pData->level >= 13) chanceofbuff = 2;
						if (pData->level >= 10 && rand() % 100 <= chanceofbuff) {
							numb += 1;
							Player::OnConsoleMessage(peer, "Harvester bonus extra block drop");
						}
						DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), world->items.at(x + (y * world->width)).foreground - 1, numb, 0);
						if (pData->quest_active && pData->lastquest == "honor" && pData->quest_step == 15 && pData->quest_progress < 100000) {
							pData->quest_progress += getItemDef(world->items.at(x + (y * world->width)).foreground).rarity * world->items.at(x + (y * world->width)).fruitcount;
							if (pData->quest_progress >= 100000) {
								pData->quest_progress = 100000;
								Player::OnTalkBubble(peer, pData->netID, "`9Legendary Quest step complete! I'm off to see a Wizard!", 0, false);
							}
						}
						world->items.at(x + (y * world->width)).spliced = false;
						world->items.at(x + (y * world->width)).growtime = 0;
						world->items.at(x + (y * world->width)).fruitcount = 0;
						if (HarvestEvent) {
							SendHarvestFestivalDrop(world, peer, x, y, getItemDef(world->items.at(x + (y * world->width)).foreground).rarity);
						}
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity < 25) {
							SendXP(peer, 1);
						} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity >= 25 && getItemDef(world->items.at(x + (y * world->width)).foreground).rarity < 40) {
							SendXP(peer, 2);
						} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity >= 40 && getItemDef(world->items.at(x + (y * world->width)).foreground).rarity < 60) {
							SendXP(peer, 3);
						} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity >= 60 && getItemDef(world->items.at(x + (y * world->width)).foreground).rarity < 80) {
							SendXP(peer, 4);
						} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity >= 80 && getItemDef(world->items.at(x + (y * world->width)).foreground).rarity < 100) {
							SendXP(peer, 5);
						} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity >= 100) {
							SendXP(peer, 6);
						}
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								treeModify(currentPeer, x, y, static_cast<PlayerInfo*>(currentPeer->data)->netID);
							}
						}
						world->items.at(x + (y * world->width)).foreground = 0;
						return;
					}
					return;
				} 
				if (!pData->Fishing) break;
				SyncFish(world, peer);
			}
		case 32: /*wrench*/
			{
				if (world->items.at(x + (y * world->width)).foreground == 1790) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						pData->choosing_quest = "";
						if (pData->lastquest == "" || !pData->quest_active) {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`9The Legendary Wizard``|left|1790|\nadd_textbox|`oGreetings, traveler! I am the Legendary Wizard. Should you wich to embark on a Legendary Quest, simply choose one below.|\nadd_spacer|small|\nadd_button|honor|`9Quest For Honor|noflags|0|0|\nend_dialog|legendary_wizard|No Thanks||");
						} else {
							send_quest_view(peer, pData, world);
						}
					}
					else if (world->owner != "") {
						Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
					}
					return;
				}
				if (world->items[x + (y * world->width)].foreground == 3528 && tile == 32) {
					if (world->owner == "" || isWorldOwner(peer, world)) {
						Player::OnDialogRequest(peer, "add_label_with_icon|big|`oArt Canvas|left|3528|\nadd_spacer|small|\nadd_textbox|`oThe Canvas is " + getItemDef(world->items[x + (y * world->width)].intdata).name + ". |\nadd_item_picker|paints|`oPaint Something|Select any item to Paint|\nadd_text_input|psign|Signed:|" + world->items[x + (y * world->width)].sign + "|100|\nend_dialog|painting|cancel|ok|");
						static_cast<PlayerInfo*>(peer->data)->wrenchedBlockLocation = x + (y * world->width);
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 4618) {
					GTDialog homeoven;
					homeoven.addLabelWithIcon("`2Home Oven", 4618, LABEL_BIG);
					homeoven.addSpacer(SPACER_SMALL);
					homeoven.addTextBox("`oWith Home Oven, you can cook some meals to not get hungry. You need to put the ingredients below. Ingredients you can get by farming `1Grass`o.");
			
					if (static_cast<PlayerInfo*>(peer->data)->myIngredient1 == "3308" || static_cast<PlayerInfo*>(peer->data)->myIngredient1Count == "0")
					{
						homeoven.addStaticBlueFrameWithIdCountText("6156", "0", "empty", "putMyIngredient_1", false);
					}
					else
					{
						homeoven.addStaticBlueFrameWithIdCountText(static_cast<PlayerInfo*>(peer->data)->myIngredient1, static_cast<PlayerInfo*>(peer->data)->myIngredient1Count, getItemDef(atoi(static_cast<PlayerInfo*>(peer->data)->myIngredient1.c_str())).name, "putMyIngredient_1", false);
					}

					if (static_cast<PlayerInfo*>(peer->data)->myIngredient2 == "3308" || static_cast<PlayerInfo*>(peer->data)->myIngredient2Count == "0")
					{
						homeoven.addStaticBlueFrameWithIdCountText("6156", "0", "empty", "putMyIngredient_2", false);
					}
					else
					{
						homeoven.addStaticBlueFrameWithIdCountText(static_cast<PlayerInfo*>(peer->data)->myIngredient2, static_cast<PlayerInfo*>(peer->data)->myIngredient2Count, getItemDef(atoi(static_cast<PlayerInfo*>(peer->data)->myIngredient2.c_str())).name, "putMyIngredient_2", false);
					}

					if (static_cast<PlayerInfo*>(peer->data)->myIngredient3 == "3308" || static_cast<PlayerInfo*>(peer->data)->myIngredient3Count == "0")
					{
						homeoven.addStaticBlueFrameWithIdCountText("6156", "0", "empty", "putMyIngredient_3", false);
					}
					else
					{
						homeoven.addStaticBlueFrameWithIdCountText(static_cast<PlayerInfo*>(peer->data)->myIngredient3, static_cast<PlayerInfo*>(peer->data)->myIngredient3Count, getItemDef(atoi(static_cast<PlayerInfo*>(peer->data)->myIngredient3.c_str())).name, "putMyIngredient_3", false);
					}

					if (static_cast<PlayerInfo*>(peer->data)->myIngredient4 == "3308" || static_cast<PlayerInfo*>(peer->data)->myIngredient4Count == "0")
					{
						homeoven.addStaticBlueFrameWithIdCountText("6156", "0", "empty", "putMyIngredient_4", false);
					}
					else
					{
						homeoven.addStaticBlueFrameWithIdCountText(static_cast<PlayerInfo*>(peer->data)->myIngredient4, static_cast<PlayerInfo*>(peer->data)->myIngredient4Count, getItemDef(atoi(static_cast<PlayerInfo*>(peer->data)->myIngredient4.c_str())).name, "putMyIngredient_4", false);
					}
					homeoven.addNewLineAfterFrame();
					homeoven.addNewLineAfterFrame();
					homeoven.addSpacer(SPACER_SMALL);
					homeoven.addButton("cookmealnow", "Cook meal");
					homeoven.addSpacer(SPACER_SMALL);
					homeoven.addTextBox("`oMeal recipes:");
					homeoven.addSpacer(SPACER_SMALL);
					string dialog2 = "";
					ifstream infile2("homeoven.txt");
					for (string line; getline(infile2, line);) {
						if (line.length() > 3 && line.at(0) != '/' && line.at(1) != '/') {
							auto ex = explode("|", line);
							dialog2 += "add_label_with_icon|small|`o" + ex.at(1) + " " + getItemDef(atoi(ex.at(0).c_str())).name + " + " + ex.at(3) + " " + getItemDef(atoi(ex.at(2).c_str())).name + " + " + ex.at(5) + " " + getItemDef(atoi(ex.at(4).c_str())).name +" + " + ex.at(7) + " " + getItemDef(atoi(ex.at(6).c_str())).name + "|left|" + ex.at(8) + "|\n";
						}
					}
					infile2.close();
					homeoven.addCustom(dialog2);
					homeoven.addSpacer(SPACER_SMALL);
					homeoven.addSpacer(SPACER_SMALL);
	
					homeoven.addQuickExit();
					homeoven.endDialog("Close", "", "Close");
					Player::OnDialogRequest(peer, homeoven.finishDialog());
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 3176) {
					GTDialog anvil;
					anvil.addLabelWithIcon("`2Anvil", 3176, LABEL_BIG);
					anvil.addSpacer(SPACER_SMALL);
					anvil.addTextBox("`oWith Anvil, you can craft lock picks. With lock picks, you can rob someone's world within 10 seconds.");
					anvil.addSpacer(SPACER_SMALL);
					anvil.addTextBox("`oTo craft `11 lock pick`o, you need these `1rocks`o: ");
					anvil.addLabelWithIcon("`110 `$"+getItemDef(3930).name,3930,LABEL_SMALL);
					anvil.addLabelWithIcon("`110 `$"+getItemDef(944).name,944,LABEL_SMALL);
					anvil.addLabelWithIcon("`110 `$"+getItemDef(1538).name,1538,LABEL_SMALL);
					anvil.addSpacer(SPACER_SMALL);
					anvil.addTextBox("`oYou can find these rocks in `1Treasure Blasted world`o.");
					anvil.addSpacer(SPACER_SMALL);
					anvil.addTextBox("`oTo craft `11 lock pick`o, click on the button bellow:");
					anvil.addButton("craftlockpick","Craft now");
					anvil.addSpacer(SPACER_SMALL);
					anvil.addTextBox("`1Addition information: `oFor each type of world lock, you need a certain number of lock picks.");
					anvil.addLabelWithIcon("`oTo rob the world, locked with `1"+getItemDef(4802).name+"`o, you need `$15 `1lock picks",4802,LABEL_SMALL);
					anvil.addLabelWithIcon("`oTo rob the world, locked with `1"+getItemDef(7188).name+"`o, you need `$10 `1lock picks",7188,LABEL_SMALL);
					anvil.addLabelWithIcon("`oTo rob the world, locked with `1"+getItemDef(1796).name+"`o, you need `$5 `1lock picks",1796,LABEL_SMALL);
					anvil.addLabelWithIcon("`oTo rob the world, locked with `1"+getItemDef(242).name+" or any other lock`o, you need `$1 `1lock pick",242,LABEL_SMALL);
					anvil.addSpacer(SPACER_SMALL);
					anvil.addSpacer(SPACER_SMALL);
					anvil.addQuickExit();
					anvil.endDialog("Close", "", "Close");
					Player::OnDialogRequest(peer, anvil.finishDialog());
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 3694) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						pData->wrenchedBlockLocation = x + (y * world->width);
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wWeather Machine - Heatwave``|left|3694|\nadd_spacer|small|\nadd_textbox|`oAdjust the color of your heat wave here, by including 0-255 of Red, Green, and Blue.|\nadd_spacer|small|\nadd_text_input|heatwave_red|Red|" + to_string(world->items.at(x + (y * world->width)).vid) + "|3|\nadd_text_input|heatwave_green|Green|" + to_string(world->items.at(x + (y * world->width)).vprice) + "|3|\nadd_text_input|heatwave_blue|Blue|" + to_string(world->items.at(x + (y * world->width)).vcount) + "|3|\nadd_quick_exit|\nend_dialog|weatherspcl9|Cancel|Okay|");
					}
					else if (world->owner != "") {
						Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 3832) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						pData->wrenchedBlockLocation = x + (y * world->width);
						string item_name = getItemDef(world->items.at(x + (y * world->width)).intdata).name;
						string gravity = to_string(world->items.at(x + (y * world->width)).mc);
						string spin_items = "0";
						if (world->items.at(x + (y * world->width)).rm) spin_items = "1";
						string invert_sky_colors = "0";
						if (world->items.at(x + (y * world->width)).opened) invert_sky_colors = "1";
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wWeather Machine - Stuff``|left|3832|\nadd_spacer|small|\nadd_item_picker|choose|Item: `2" + item_name + "``|Select any item to rain down|\nadd_text_input|gravity|Gravity:|" + gravity + "|5|\nadd_checkbox|spin|Spin Items|" + spin_items + "\nadd_checkbox|invert|Invert Sky Colors|" + invert_sky_colors + "\nend_dialog|weatherspcl|Cancel|Okay|");
					}
					else if (world->owner != "") {
						Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 5000) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						pData->wrenchedBlockLocation = x + (y * world->width);
						string item_name = getItemDef(world->items.at(x + (y * world->width)).intdata).name;
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wWeather Machine - Background``|left|5000|\nadd_spacer|small|\nadd_textbox|You can scan any Background Block to set it up in your weather machine.|left|\nadd_item_picker|choose|Item: `2" + item_name + "``|Select any Background Block|\nend_dialog|weatherspcl3|Cancel|Okay|");
					}
					else if (world->owner != "") {
						Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 9150) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wMadenci``|left|9150|\nadd_textbox|Hosgeldin, genc madenci! Bugun seni cok enerjik ve heyecanli goruyorum. Bugun ne yapmak istersin?|left|\nadd_button|chc0|Madenci Itemleri|noflags|0|0|\nadd_button|chc1|Maden Satisi Yap|noflags|0|0|\nend_dialog|phonecall|Cikis||");
					} else if (world->owner != "") {
						Player::OnTalkBubble(peer, pData->netID, "Buranin sahibi " + world->owner + "", 0, true);
					}
					return;
				}
		 		if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
					pData->wrenchedBlockLocation = x + (y * world->width);
					if (world->category == "Guild" && world->items.at(x + (y * world->width)).foreground == 5814) {
						
						if (pData->guild == "") {
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}
						GuildInfo* guild = guildDB.get_pointer(pData->guild);
						if (guild == NULL)
						{
							Player::OnConsoleMessage(peer, "`4An error occurred while getting guilds information! `1(#1)`4.");
							return;
						}
						if (guild->world != world->name)
						{
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}

						if (isWorldOwner(peer, world))
						{
							GTDialog guildlockforowner;
							guildlockforowner.addCustom(GetGuildCustomDialogTextForGuildLabel(guild, "`wGuild Lock``", "big"));

							guildlockforowner.addCustom("add_spacer|small|");
							guildlockforowner.addCustom("add_player_info|" + guild->name + " | " + to_string(guild->level) + "|" + to_string(guild->xp) + "|" + to_string(GetMaxGuildXp(guild->level)) + "|");
							guildlockforowner.addCustom("add_spacer|small|");
							if (guild->level < 15)
							{
								if (guild->xp >= GetMaxGuildXp(guild->level))
								{
									guildlockforowner.addCustom("add_button|upgradeguildconfirm|`wUpgrade Guild``|noflags|0|0|");
									guildlockforowner.addCustom("add_textbox|Cost: `2" + to_string(GetGuildUpgradePrice(guild->level)) + " Gems``|left|");
									guildlockforowner.addCustom("add_spacer|small|");
								}
							}
							guildlockforowner.addCustom("add_textbox|`wManage Guild Member access:``|left|");
							guildlockforowner.addCustom("add_checkbox|checkbox_coleader|Enable Co-Leader access|"+to_string(guild->accesslockCoLeader)+"");
							guildlockforowner.addCustom("add_checkbox|checkbox_elder|Enable Co-Leader and Elder access|" + to_string(guild->accesslockCoLeaderAndElder) + "");
							guildlockforowner.addCustom("add_checkbox|checkbox_member|Enable all Members access|" + to_string(guild->accesslockAllMembers) + "");
							guildlockforowner.addCustom("add_spacer|small|");
							guildlockforowner.addCustom("add_spacer|small|");

							string str = "";
							for (auto i = 0; i < world->accessed.size(); i++) {
								if (world->accessed.at(i) == "") continue;
								str += "\nadd_checkbox|checkbox_" + world->accessed.at(i) + "|" + world->accessed.at(i) + "|1";
							}
							if (str != "")
							{
								guildlockforowner.addCustom(str);
								guildlockforowner.addCustom("add_spacer|small|");
							}
							else
							{
								guildlockforowner.addCustom("add_label|small|Currently, you're the only one with access.``|left\nadd_spacer|small|");
							}
							guildlockforowner.addCustom("add_player_picker|playerNetID|`wAdd``|");
							guildlockforowner.addCustom("add_spacer|small|");
							if (guild->level >= 3 && guild->members.size() >= 15)
							{
								guildlockforowner.addCustom("add_button|btneditguildmascot|`wChange Guild Mascot``|noflags|0|0|");
							}
							else
							{
								guildlockforowner.addCustom("add_textbox|``Increase your guild size to `w15 ``or more members and you can set a guild mascot!``|left|");
							}
							guildlockforowner.addCustom("add_spacer|small|");

							string guildkey = "";
							auto iscontains = false;
							SearchInventoryItem(peer, 5816, 1, iscontains);
							if (!iscontains) {
								guildkey = "\nadd_button|getGuildKey|Get Guild Key|noflags|0|0|";
							}

							guildlockforowner.addCustom("`wEnable/Disable Collecting Items``|\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|" + to_string(world->isPublic) + "\nadd_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0|\nadd_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|\nadd_checkbox|checkbox_muted|Silence, Peasants!|" + to_string(world->silence) + "|noflags|0|0|\nadd_text_input|minimum_entry_level|World Level: |" + to_string(world->entrylevel) + "|3|\nadd_smalltext|Set minimum world entry level.|"+ guildkey +"");
							guildlockforowner.addCustom("add_button|abandon_guild|`wAbandon Guild``|noflags|0|0|");
							
							guildlockforowner.addQuickExit();
							guildlockforowner.endDialog("editguildlock", "OK", "Cancel");
							Player::OnDialogRequest(peer, guildlockforowner.finishDialog());
							return;
						}

						GuildRanks myrank = GuildGetRank(guild, pData->rawName);
						if (guild->accesslockAllMembers || isWorldAdmin(peer, world)) {}
						else if (!guild->accesslockAllMembers && !guild->accesslockCoLeaderAndElder && !guild->accesslockCoLeader)
						{
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}
						else if ((guild->accesslockCoLeaderAndElder || guild->accesslockCoLeader) && myrank == GuildRanks::Member)
						{
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}
						else if (guild->accesslockCoLeader && !guild->accesslockCoLeaderAndElder && myrank != GuildRanks::Co)
						{
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}
						else if (guild->accesslockCoLeader && guild->accesslockCoLeaderAndElder && (myrank != GuildRanks::Co && myrank != GuildRanks::Elder))
						{
							Player::OnTalkBubble(peer, pData->netID, "`oI'm `4unable `oto pick the lock.", 0, true);
							return;
						}
						GTDialog guildlock;
						guildlock.addCustom(GetGuildCustomDialogTextForGuildLabel(guild, "`wGuild Lock``", "big"));

						guildlock.addCustom("add_spacer|small|");
						guildlock.addCustom("add_player_info|"+guild->name+" | "+to_string(guild->level)+"|"+to_string(guild->xp)+"|"+to_string(GetMaxGuildXp(guild->level))+"|");
						guildlock.addCustom("add_spacer|small|");
						guildlock.addCustom("add_textbox|Category: "+world->category+"|left|");
						guildlock.addCustom("add_label|small|This guild is lead by `w"+guild->leader+"``, but I have been given access to the guild world.|left");
						if (isWorldAdmin(peer, world))  guildlock.addButton("removeaccessfromworld","Remove My Access");
						guildlock.addQuickExit();
						guildlock.endDialog("Close", "", "Exit");
						Player::OnDialogRequest(peer, guildlock.finishDialog());
						return;
					}
					if (isWorldOwner(peer, world) || pData->rawName == world->items.at(x + (y * world->width)).monitorname) {				
						if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
							string allow_dialog = "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|0";
							if (world->items.at(x + (y * world->width)).opened) {
								allow_dialog = "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|1";
							}
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit Small Lock``|left|202|\nadd_label|small|`wAccess list:``|left\nadd_spacer|small|\nadd_label|small|Currently, you're the only one with access.``|left\nadd_spacer|small|\nadd_player_picker|playerNetID|`wAdd``|" + allow_dialog + "\nadd_checkbox|checkbox_ignore|Ignore empty air|0\nadd_button|recalcLock|`wRe-apply lock``|noflags|0|0|\nend_dialog|lock_edit|Cancel|OK|");
							return;
						}
						int ispub = world->isPublic;
						string str = "";
						for (auto i = 0; i < world->accessed.size(); i++) {
							if (world->accessed.at(i) == "") continue;
							str += "\nadd_checkbox|checkbox_" + world->accessed.at(i) + "|"+world->accessed.at(i)+"|1";
						}
						string drop_gems = "0|";
						if (world->rating == 1) drop_gems = "1|";
						int muted = world->silence;
						string wlmenu = "";
						string category_change = "\nadd_button|changecat|`wCategory: " + world->category + "``|noflags|0|0|";
						string abandon_guild = "";
						string change_guild_fg = "";
						string change_guild_bg = "";
						string clear_guild_mascot = "";
						string world_key = "";
						auto iscontains = false;
						SearchInventoryItem(peer, 1424, 1, iscontains);
						if (!iscontains) {
							world_key = "\nadd_button|getKey|Get World Key|noflags|0|0|";
						}

						string premium = "";
						if (std::find(premium_worlds.begin(), premium_worlds.end(), world->name) == premium_worlds.end() && std::find(diamond_worlds.begin(), diamond_worlds.end(), world->name) == diamond_worlds.end() && std::find(platinum_worlds.begin(), platinum_worlds.end(), world->name) == platinum_worlds.end()) {			
							premium = "";
						} else {
							if (std::find(diamond_worlds.begin(), diamond_worlds.end(), world->name) == diamond_worlds.end() && std::find(platinum_worlds.begin(), platinum_worlds.end(), world->name) == platinum_worlds.end()) {			
								premium = "";
							} else {
								if (std::find(platinum_worlds.begin(), platinum_worlds.end(), world->name) == platinum_worlds.end()) {			
									premium = "";
								}
							}
						}
						if (world->items.at(x + (y * world->width)).foreground == 4802) {
							string rainbows = "0";
							if (world->rainbow) rainbows = "1";
							if (str == "") {
								wlmenu = "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_label|small|`wAccess list:``|left\nadd_spacer|small|\nadd_label|small|Currently, you're the only one with access.``|left\nadd_spacer|small|\nadd_player_picker|playerNetID|`wAdd``|\nadd_button|WorldDropPickup|`wEnable/Disable Collecting Items``|" + premium + "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|" + to_string(ispub) + "\nadd_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0|\nadd_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|\nadd_checkbox|checkbox_rainbow|Enable Rainbows!|" + rainbows + "\nadd_checkbox|checkbox_muted|Silence, Peasants!|" + to_string(muted) + "|noflags|0|0|\nadd_text_input|minimum_entry_level|World Level: |1|3|\nadd_smalltext|Set minimum world entry level.|"+ category_change + world_key + "\nend_dialog|lock_edit|Cancel|OK|";
							} else {
								wlmenu = "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_label|small|`wAccess list:``|left\nadd_spacer|small|\nadd_label|small|" + str + "|left\nadd_spacer|small|\nadd_player_picker|playerNetID|`wAdd``|\nadd_button|WorldDropPickup|`wEnable/Disable Collecting Items``|" + premium + "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|" + to_string(ispub) + "\nadd_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0|\nadd_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|\nadd_checkbox|checkbox_rainbow|Enable Rainbows!|" + rainbows + "\nadd_checkbox|checkbox_muted|Silence, Peasants!|" + to_string(muted) + "|noflags|0|0|\nadd_text_input|minimum_entry_level|World Level: |1|3|\nadd_smalltext|Set minimum world entry level.|" + category_change + world_key + "\nend_dialog|lock_edit|Cancel|OK|";
							}
							Player::OnDialogRequest(peer, wlmenu);
						} else {
							if (str == "") {
								wlmenu = "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_label|small|`wAccess list:``|left\nadd_spacer|small|\nadd_label|small|Currently, you're the only one with access.``|left\nadd_spacer|small|\nadd_player_picker|playerNetID|`wAdd``|\nadd_button|WorldDropPickup|`wEnable/Disable Collecting Items``|" + premium + "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|" + to_string(ispub) + "\nadd_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0|\nadd_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|\nadd_text_input|minimum_entry_level|World Level: |" + to_string(world->entrylevel) + "|3|\nadd_smalltext|Set minimum world entry level.|" + category_change + world_key + "\nend_dialog|lock_edit|Cancel|OK|";
							} else {
								wlmenu = "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_label|small|`wAccess list:``|left\nadd_spacer|small|\nadd_label|small|" + str + "|left\nadd_spacer|small|\nadd_player_picker|playerNetID|`wAdd``|\nadd_button|WorldDropPickup|`wEnable/Disable Collecting Items``|" + premium + "\nadd_checkbox|checkbox_public|Allow anyone to Build and Break|" + to_string(ispub) + "\nadd_checkbox|checkbox_disable_music|Disable Custom Music Blocks|0|\nadd_checkbox|checkbox_disable_music_render|Make Custom Music Blocks invisible|\nadd_text_input|minimum_entry_level|World Level: |" + to_string(world->entrylevel) + "|3|\nadd_smalltext|Set minimum world entry level.|" + category_change + world_key + "\nend_dialog|lock_edit|Cancel|OK|";
							}
							Player::OnDialogRequest(peer, wlmenu);
						}
					} else {
						if (isWorldAdmin(peer, world)) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								try {
									ifstream read_player("save/players/_" + whoslock + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + whoslock;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`2Access Granted`w)", 0, true);
									Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
								return;
							}
						} else if (world->isPublic) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								try {
									ifstream read_player("save/players/_" + whoslock + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + whoslock;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9Open to Public`w)", 0, true);
									Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
								return;
							}
						} else if (world->isEvent) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								if (pData->rawName != whoslock) {
									try {
										ifstream read_player("save/players/_" + whoslock + ".json");
										if (!read_player.is_open()) {
											return;
										}		
										json j;
										read_player >> j;
										read_player.close();
										string nickname = j["nick"];
										int adminLevel = j["adminLevel"];
										if (nickname == "") {
											nickname = role_prefix.at(adminLevel) + whoslock;
										} 
										if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9Open to Public`w)", 0, true);
										else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4No Access`w)", 0, true);
										Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
									} catch (std::exception& e) {
										std::cout << e.what() << std::endl;
										return;
									}
									return;
								}
							}
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 6952) {
					if (world->isPublic) return; 
					if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner) || isDev(peer)) {
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						string PoolDialog = "\nadd_item_picker|autobreakitem|No block type selected!|Choose the target item!|";
						string StopDialog = "";
						if (world->items.at(x + (y * world->width)).mid != 0) {
							PoolDialog = "\nadd_item_picker|autobreakitem|Target block is: `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name +"``|Choose the target item!|";
							StopDialog = "\nadd_button|manipulatorstop|Stop the machine|";
						}
						string AutoHarvestDialog = "\nadd_checkbox|checkbox_autoharvest|`oAuto harvest trees|0|";
						if (world->items.at(x + (y * world->width)).rm) {
							AutoHarvestDialog = "\nadd_checkbox|checkbox_autoharvest|`oAuto harvest trees|1|";
						}
						Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_textbox|`oThis machine will break blocks for you! Select one and it will start breaking it immediately! Just make sure to select the right one|left||" + PoolDialog + "|" + StopDialog + "|" + AutoHarvestDialog + "\nend_dialog|autobreak|Close|OK|");
					}
					return;
				}

				if (world->items.at(x + (y * world->width)).foreground == 6954) {
					if (world->isPublic) return; 
					if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner) || isDev(peer)) {
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						string PoolDialog = "\nadd_item_picker|autoplaceitem|No auto place block selected!|Choose the target block!|";
						string addthemdialog = "";
						string stopdialog = "";
						if (world->items.at(x + (y * world->width)).mid != 0) {
							PoolDialog = "\nadd_item_picker|autoplaceitem|Target auto place block is: `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name +"`` (`2" + to_string(world->items.at(x + (y * world->width)).mc) + "/5000``)|Choose the target block!|";
							auto mtitems = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++) {
								if (pData->inventory.items.at(i).itemID == world->items.at(x + (y * world->width)).mid) {
									mtitems = pData->inventory.items.at(i).itemCount;
									break;
								}
							}
							if (mtitems != 0 && world->items.at(x + (y * world->width)).mc + mtitems <= 5000) {
								addthemdialog = "\nadd_smalltext|`oYou have " + to_string(mtitems) + " " + getItemDef(world->items.at(x + (y * world->width)).mid).name + " in your backpack.``|left|\nadd_button|addorganic|Add them to the machine|";
							}
							if (world->items.at(x + (y * world->width)).mc > 0) stopdialog = "\nadd_button|organicstop|Retrieve Items|";
						}
						string PoolDialogs = "\nadd_item_picker|autoplaceidentityitem|No ident block selected!|Choose the identification block!|";
						if (world->items.at(x + (y * world->width)).vid != 0) {
							PoolDialogs = "\nadd_item_picker|autoplaceidentityitem|Target ident block is: `2" + getItemDef(world->items.at(x + (y * world->width)).vid).name +"``|Choose the identification block!|";
						}
						string MagplantsStoragesList = "";
						string UseMagDialog = "";
						if (world->items.at(x + (y * world->width)).mid != 0) {
							for (auto i = 0; i < world->width * world->height; i++) {
								if (world->items.at(i).foreground == 5638 && world->items.at(i).mid == world->items.at(x + (y * world->width)).mid) {
									MagplantsStoragesList += "\nadd_label_with_icon|small|`2" + getItemDef(world->items.at(i).mid).name + " `oMagplant (`2" + to_string(world->items.at(i).mc) + "`o)|left|" + to_string(world->items.at(i).mid) + "|";
								}
							}
							if (MagplantsStoragesList != "") {
								UseMagDialog = "\nadd_checkbox|checkbox_publicremote|`oUse magplants storage with the " + getItemDef(world->items.at(x + (y * world->width)).vid).name + "|0|";
								if (world->items.at(x + (y * world->width)).rm) {
									UseMagDialog = "\nadd_checkbox|checkbox_publicremote|`oUse magplants storage with the " + getItemDef(world->items.at(x + (y * world->width)).vid).name + "|1|";
								}
							}
						}
						Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_textbox|`oSelect the block you want this machine to automatically place for you, also you will need to select the second foreground block which will be used as identification for the first one, let's say we will select that we want to place Dirt automatically, and in the identification block we will set Wooden Platform, so that will cause our Dirt to placed above Wooden Platforms in your world!|left||" + PoolDialog + "|" + addthemdialog + "|" + PoolDialogs + "|" + MagplantsStoragesList + "|" + UseMagDialog +"|" + stopdialog + "\nend_dialog|autoplace|Close|OK|");
					}
					return;
				}

				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SUCKER) {
					if (world->isPublic) return; 
					auto squaresign = x + (y * world->width);
					auto currentworld = pData->currentWorld + "X" + std::to_string(squaresign);
					string Sucker = "";
					if (world->items.at(x + (y * world->width)).foreground == 5638) {
						Sucker = "magplant";
						pData->suckername = "magplant";
						pData->suckerid = 5638;
					}
					if (world->items.at(x + (y * world->width)).foreground == 6946) {
						Sucker = "gaiabeacon";
						pData->suckername = "gaiabeacon";
						pData->suckerid = 6946;
					}
					if (world->items.at(x + (y * world->width)).foreground == 6948) {
						Sucker = "unstabletesseract";
						pData->suckername = "unstabletesseract";
						pData->suckerid = 6948;
					}
					pData->lastPunchX = x;
					pData->lastPunchY = y;
					if (pData->rawName != world->owner && !isDev(peer)) {
						if (world->items.at(x + (y * world->width)).rm) {
							string label = "`6The machine is currently empty!";
							string RemoteDialog = "";
							if (world->items.at(x + (y * world->width)).mc >= 1) {
								RemoteDialog = "\nadd_textbox|`oUse the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " Remote to build `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name + " `odirectly from the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "'s storage.|\nadd_button|getremote|`oGet Remote|";
								label = "`oThe machine contains " + to_string(world->items.at(x + (y * world->width)).mc) + " `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name + ""; /*The message if something exists in item sucker*/
							}
							Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_spacer|small|\nadd_label_with_icon|small|`5" + getItemDef(world->items.at(x + (y * world->width)).mid).name + "|left|" + to_string(world->items.at(x + (y * world->width)).mid) + "|\nadd_textbox|" + label + "|" + RemoteDialog + "|\nend_dialog|magplantupdate|Close||");
						}
						else {
							Player::OnTalkBubble(peer, pData->netID, "This magplant public mode is disabled!", 0, false);
						}
						return;
					}
					if (world->items.at(x + (y * world->width)).mid != 0) {
						string label = "`6The machine is currently empty!"; 
						string RemoteDialog = "";
						string publicremotecheck = "";
						string gemDialog = "";
						bool Farmable = false;
						if (world->items.at(x + (y * world->width)).mid == 7382 || world->items.at(x + (y * world->width)).mid == 4762 || world->items.at(x + (y * world->width)).mid == 5140 || world->items.at(x + (y * world->width)).mid == 5138 || world->items.at(x + (y * world->width)).mid == 5136 || world->items.at(x + (y * world->width)).mid == 5154 || world->items.at(x + (y * world->width)).mid == 340 || world->items.at(x + (y * world->width)).mid == 954 || world->items.at(x + (y * world->width)).mid == 5666) Farmable = true;
						if (Sucker == "magplant" && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::FOREGROUND || Sucker == "magplant" && Farmable || Sucker == "magplant" && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::BACKGROUND || Sucker == "magplant" && getItemDef(world->items.at(x + (y * world->width)).mid).blockType == BlockTypes::GROUND_BLOCK) {
							if (getItemDef(world->items.at(x + (y * world->width)).mid).properties & Property_AutoPickup) {
								/*...*/
							} else {
								if (world->items.at(x + (y * world->width)).rm) {
									publicremotecheck = "\nadd_checkbox|checkbox_publicremote|`oAllow visitors to take remote|1|";
								}
								else {
									publicremotecheck = "\nadd_checkbox|checkbox_publicremote|`oAllow visitors to take remote|0|";
								}
								RemoteDialog = "\nadd_textbox|`oUse the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " Remote to build `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name + " `odirectly from the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "'s storage.|\nadd_button|getremote|`oGet Remote|";
							}
						}
						if (Sucker == "magplant") {
							if (world->items.at(x + (y * world->width)).mid == 112) {
								gemDialog = "\nadd_checkbox|checkbox_gemcollection|`oEnable gem collection|1|";
							} else {
								gemDialog = "\nadd_checkbox|checkbox_gemcollection|`oEnable gem collection|0|";
							}
						}
						if (world->items.at(x + (y * world->width)).mc >= 1) {
							label = "`oThe machine contains " + to_string(world->items.at(x + (y * world->width)).mc) + " `2" + getItemDef(world->items.at(x + (y * world->width)).mid).name + ""; /*The message if something exists in item sucker*/
							Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_spacer|small|\nadd_label_with_icon|small|`5" + getItemDef(world->items.at(x + (y * world->width)).mid).name + "|left|" + to_string(world->items.at(x + (y * world->width)).mid) + "|\nadd_textbox|" + label + "|\nadd_button|retrieveitem|`oRetrieve Items|" + RemoteDialog + "|" + publicremotecheck + "|\nend_dialog|magplantupdate|Close|`wUpdate|");
						}
						else {
							Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_spacer|small|\nadd_label_with_icon|small|`5" + getItemDef(world->items.at(x + (y * world->width)).mid).name + "|left|" + to_string(world->items.at(x + (y * world->width)).mid) + "|\nadd_textbox|" + label + "|\nadd_item_picker|magplantitem|Change Item|Choose an item to put in the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "!|" + publicremotecheck + "|" + gemDialog + "\nend_dialog|magplantupdate|Close|`wUpdate|");
						}
					}
					else {
						if (Sucker == "magplant") Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_spacer|small|\nadd_textbox|`6The machine is empty.|\nadd_item_picker|magplantitem|Choose Item|Choose an item to put in the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "!|\nadd_checkbox|checkbox_gemcollection|`oEnable gem collection|0|\nend_dialog|magplantcheck|Close|`wUpdate|");
						else Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`w" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_spacer|small|\nadd_textbox|`6The machine is empty.|\nadd_item_picker|magplantitem|Choose Item|Choose an item to put in the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "!|\nend_dialog|magplant|Close||");
					}
					return;
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::PORTAL) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						pData->wrenchsession = x + (y * world->width);
						string DestWorldDialog = world->items.at(x + (y * world->width)).destWorld;
						if (world->items.at(x + (y * world->width)).destId != "") {
							DestWorldDialog += ":" + world->items.at(x + (y * world->width)).destId;
						}
						string IdDialog = "\nadd_text_input|door_id|ID|" + world->items.at(x + (y * world->width)).currId + "|11|\nadd_smalltext|Set a unique `2ID`` to target this door as a Destination from another!|left|";
						if (world->items.at(x + (y * world->width)).foreground == 762) {
							IdDialog = "\nadd_text_input|door_id|Password|" + world->items.at(x + (y * world->width)).password + "|23|";
						}
						if (!world->items.at(x + (y * world->width)).opened) {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_text_input|door_name|Label|" + world->items.at(x + (y * world->width)).label + "|100|\nadd_text_input|door_target|Destination|" + DestWorldDialog + "|24|\nadd_smalltext|Enter a Destination in this format: `2WORLDNAME:ID``|left|\nadd_smalltext|Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.|left|" + IdDialog + "|\nadd_checkbox|checkbox_public|Is open to public|0\nend_dialog|door_edit|Cancel|OK|");
						}
						else {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_text_input|door_name|Label|" + world->items.at(x + (y * world->width)).label + "|100|\nadd_text_input|door_target|Destination|" + DestWorldDialog + "|24|\nadd_smalltext|Enter a Destination in this format: `2WORLDNAME:ID``|left|\nadd_smalltext|Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.|left|" + IdDialog + "|\nadd_checkbox|checkbox_public|Is open to public|1\nend_dialog|door_edit|Cancel|OK|");
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 3818) {
					if (world->owner == "" || isWorldOwner(peer, world)) {
						if (world->items[x + (y * world->width)].intdata != 0 && isWorldOwner(peer, world))
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wEdit Portrait``|left|3818|0|0|\nadd_spacer|small|\nadd_textbox|`oThis is a lovely portrait of a GrowTopian.|left|\nadd_textbox|`oYou'll need 4 Paint Bucket - Varnish to erase this|left|\nadd_text_input|artsign|Signed: |" + getItemDef(world->items[x + (y * world->width)].intdata).name + "|35|left|\nadd_smalltext|`oIf you'd like to touch up the painting slightly. you could change the expression:|left|\nadd_checkbox|unconcerned|Unconcerned|\nadd_checkbox|happy|Happy|\nend_dialog|portrait|Cancel|Okay|");
							static_cast<PlayerInfo*>(peer->data)->wrenchedBlockLocation = x + (y * world->width);
						}
						else if (getPlyersWorld(peer)->owner == "" && world->items[x + (y * world->width)].intdata != 0)
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wEdit Portrait``|left|3818|0|0|\nadd_spacer|small|\nadd_textbox|`oThis is a lovely portrait of a GrowTopian.|left|\nadd_textbox|`oYou'll need 4 Paint Bucket - Varnish to erase this|left|\nadd_text_input|artsign|Title: |" + getItemDef(world->items[x + (y * world->width)].intdata).name + "|35|left|\nadd_smalltext|`oIf you'd like to touch up the painting slightly. you could change the expression:|left|\nadd_checkbox|unconcerned|Unconcerned|\nadd_checkbox|happy|Happy|\nend_dialog|portrait|Cancel|Okay|");
							static_cast<PlayerInfo*>(peer->data)->wrenchedBlockLocation = x + (y * world->width);
						}
						else
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wPortrait``|left|3818|0|0|\nadd_spacer|small|\nadd_textbox|`oThe canvas is blank.|left|\nadd_player_picker|addportrait|`wPaint Someone|\nadd_smalltext|`5(Painting costs 2 Paint Bucket of each color)|left|\nend_dialog|portrait|Cancel|Okay|");
							static_cast<PlayerInfo*>(peer->data)->wrenchedBlockLocation = x + (y * world->width);
						}
					}
				}
				if (world->items[x + (y * world->width)].foreground == 3798) {
					if (isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || world->owner == "" || !restricted_area(peer, world, x, y)) {
						((PlayerInfo*)(peer->data))->wrenchedBlockLocation = x + (y * world->width);
						string str = "";
						string offname = "";
						for (std::vector<string>::const_iterator i = world->items[x + (y * world->width)].vipentranceaccess.begin(); i != world->items[x + (y * world->width)].vipentranceaccess.end(); ++i) {
							offname = *i;
							str += "\nadd_checkbox|unaccess_" + offname + "|" + offname + "|1|\n";
						}
						
						if (str == "") {
							str = "\nadd_label|small|`oNobody!|left|\nadd_spacer|small|";
						}
						Player::OnDialogRequest(peer, "\nadd_label_with_icon|big|`wEdit VIP Entrance|left|3798|\nadd_label|small|`wVIP List:|left|\nadd_spacer|small|" + str + "\nadd_player_picker|playerNetID|`wAdd``|\nadd_spacer|small|\nadd_checkbox|checkbox_public|Allow anyone to enter|" + to_string(world->items[x + (y * world->width)].opened) + "|\nend_dialog|vip_edit|Cancel|OK|");
					}
				}

				if (getItemDef(world->items[x + (y * world->width)].foreground).blockType == BlockTypes::SHELF) {
					if (isDev(peer) || isWorldOwner(peer, world) || isWorldAdmin(peer, world)) {
						((PlayerInfo*)(peer->data))->wrenchedBlockLocation = x + (y * world->width);
						int shelf1 = 0;
						int shelf2 = 0;
						int shelf3 = 0;
						int shelf4 = 0;
						string dshelf1 = "";
						string dshelf2 = "";
						string dshelf3 = "";
						string dshelf4 = "";
						ifstream dshelf("save/dshelf/" + ((PlayerInfo*)(peer->data))->currentWorld + "/X" + std::to_string(squaresign) + ".txt");
						dshelf >> shelf1;
						dshelf >> shelf2;
						dshelf >> shelf3;
						dshelf >> shelf4;
						dshelf.close();
						if (shelf1 != 0) {
							dshelf1 = "\nadd_button|take1|`oTake this item|left|";
						}
						else {
							dshelf1 = "\nadd_item_picker|shelf1|`4Display an item|Choose any item you want to display in shelf|";
						}
						if (shelf2 != 0) {
							dshelf2 = "\nadd_button|take2|`oTake this item|left|";
						}
						else {
							dshelf2 = "\nadd_item_picker|shelf2|`4Display an item|Choose any item you want to display in shelf|";
						}
						if (shelf3 != 0) {
							dshelf3 = "\nadd_button|take3|`oTake this item|left|";
						}
						else {
							dshelf3 = "\nadd_item_picker|shelf3|`4Display an item|Choose any item you want to display in shelf|";
						}
						if (shelf4 != 0) {
							dshelf4 = "\nadd_button|take4|`oTake this item|left|";
						}
						else {
							dshelf4 = "\nadd_item_picker|shelf4|`4Display an item|Choose any item you want to display in shelf|";
						}
						Player::OnDialogRequest(peer, "\nadd_label_with_icon|big|`$" + getItemDef(world->items[x + (y * world->width)].foreground).name + "|left|" + to_string(world->items[x + (y * world->width)].foreground) + "|left|\nadd_spacer|small|" + dshelf1 + "" + dshelf2 + "" + dshelf3 + "" + dshelf4 + "\nadd_spacer|small|\nadd_quick_exit|\nend_dialog||Okay|");
					}
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SIGN || world->items.at(x + (y * world->width)).foreground == 4722 || world->items.at(x + (y * world->width)).foreground == 6214 || world->items.at(x + (y * world->width)).foreground == 1420) {
					if (world->owner == "" || isWorldOwner(peer, world) || isWorldAdmin(peer, world) || isDev(peer) || !restricted_area(peer, world, x, y)) {
						auto &signtext = world->items.at(x + (y * world->width)).sign;
						pData->wrenchedBlockLocation = x + (y * world->width);
						if (world->items.at(x + (y * world->width)).foreground == 6214 || world->items.at(x + (y * world->width)).foreground == 1420) {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_textbox|To dress, select a clothing item then use on the mannequin. To remove clothes, punch it or select which item to remove.<CR><CR>It will go into your backpack if you have room.|\nadd_textbox|<CR><CR>What would you like to write on its sign?``|left|\nadd_text_input|sign_textas||" + signtext + "|128|\nend_dialog|mannequin_edit|Cancel|OK|");
							return;
						}
						else if (world->items.at(x + (y * world->width)).foreground == 4722) {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_textbox|Adventure Message:``|left|\nadd_text_input|adventuretextas||" + signtext + "|128|\nend_dialog|editadventure|Cancel|OK|");
							return;
						}
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|\nadd_textbox|What would you like to write on this sign?``|left|\nadd_text_input|signtextas||" + signtext + "|128|\nend_dialog|editsign|Cancel|OK|");
					}
					return;
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::GATEWAY) {
					if (isWorldOwner(peer, world) || world->owner == "" || isWorldAdmin(peer, world) || isDev(peer) || !restricted_area(peer, world, x, y)) {
						if (world->owner == "") {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_textbox|This object has additional properties to edit if in a locked area.|left|\nend_dialog|gateway_edit|Cancel|OK|");
							return;
						}
						pData->wrenchx = x;
						pData->wrenchy = y;
						if (!world->items.at(x + (y * world->width)).opened) {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_checkbox|checkbox_public|Is open to public|0\nend_dialog|gateway_edit|Cancel|OK|");
						} else {
							Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wEdit " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "``|left|" + to_string(getItemDef(world->items.at(x + (y * world->width)).foreground).id) + "|\nadd_checkbox|checkbox_public|Is open to public|1\nend_dialog|gateway_edit|Cancel|OK|");				
						}
					}
					return;
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::VENDING)
				{
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						if (world->owner != "" && !isWorldOwner(peer, world) && !isDev(peer))
						{
							SendBuyerVendDialog(peer, world);
							return;
						}
						SendVendDialog(peer, world);
					} else {
						Player::OnTalkBubble(peer, pData->netID, "Get closer!", 0, false);
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 4296) {
					if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer)) {
						if (pData->SurgeryCooldown) {
							Player::OnTalkBubble(peer, pData->netID, "I know it's just a robot, but the authorities don't even trust you operating on that with your malpractice issues.", 0, true);
							return;
						}
						if (pData->PerformingSurgery) end_surgery(peer, true);
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						string surgerywarning = "";
						vector<int> Tools;
						SearchInventoryItem(peer, 1258, 20, iscontains);
						if (!iscontains) {
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++) {
								if (pData->inventory.items.at(i).itemID == 1258 && pData->inventory.items.at(i).itemCount >= 1) {
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							if (KiekTuri != 0) Tools.push_back(1258);
						}
						else Tools.push_back(1258);
						SearchInventoryItem(peer, 1260, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1260 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Scalpel ";

							if (KiekTuri != 0) Tools.push_back(1260);

						}
						else Tools.push_back(1260);
						SearchInventoryItem(peer, 1262, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1262 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Anesthetic ";

							if (KiekTuri != 0) Tools.push_back(1262);

						}
						else Tools.push_back(1262);
						SearchInventoryItem(peer, 1264, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1264 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Antiseptic ";

							if (KiekTuri != 0) Tools.push_back(1264);

						}
						else Tools.push_back(1264);
						SearchInventoryItem(peer, 1266, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1266 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Antibiotics ";

							if (KiekTuri != 0 && pData->UnlockedAntibiotic) Tools.push_back(1266);

						}
						else if (pData->UnlockedAntibiotic) Tools.push_back(1266);
						SearchInventoryItem(peer, 1268, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1268 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Splint ";

							if (KiekTuri != 0) Tools.push_back(1268);

						}
						else Tools.push_back(1268);
						SearchInventoryItem(peer, 1270, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 1270 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Stitches ";

							if (KiekTuri != 0) Tools.push_back(1270);

						}
						else Tools.push_back(1270);
						SearchInventoryItem(peer, 4308, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4308 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Pins ";

							if (KiekTuri != 0) Tools.push_back(4308);

						}
						else Tools.push_back(4308);
						SearchInventoryItem(peer, 4310, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4310 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Transfusion ";

							if (KiekTuri != 0) Tools.push_back(4310);

						}
						else Tools.push_back(4310);
						SearchInventoryItem(peer, 4312, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4312 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Defibrillator ";

							if (KiekTuri != 0 && pData->PatientHeartStopped) Tools.push_back(4312);

						}
						else if (pData->PatientHeartStopped) Tools.push_back(4312);
						SearchInventoryItem(peer, 4314, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4314 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Clamp ";

							if (KiekTuri != 0) Tools.push_back(4314);

						}
						else Tools.push_back(4314);
						SearchInventoryItem(peer, 4316, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4316 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Ultrasound ";

							if (KiekTuri != 0) Tools.push_back(4316);

						}
						else Tools.push_back(4316);
						SearchInventoryItem(peer, 4318, 20, iscontains);
						if (!iscontains)
						{
							auto KiekTuri = 0;
							for (auto i = 0; i < pData->inventory.items.size(); i++)
							{
								if (pData->inventory.items.at(i).itemID == 4318 && pData->inventory.items.at(i).itemCount >= 1)
								{
									KiekTuri = pData->inventory.items.at(i).itemCount;
								}
							}
							//surgerywarning += "`4" + to_string(KiekTuri) + "`` Surgical Lab Kit";

							if (KiekTuri != 0) Tools.push_back(4318);

						}
						else Tools.push_back(4318);

						for (int i = 0; i < Tools.size(); i++)
						{
							if (i == 0) pData->SurgItem1 = Tools.at(i);
							if (i == 1) pData->SurgItem2 = Tools.at(i);
							if (i == 2) pData->SurgItem3 = Tools.at(i);
							if (i == 3) pData->SurgItem4 = Tools.at(i);
							if (i == 4) pData->SurgItem5 = Tools.at(i);
							if (i == 5) pData->SurgItem6 = Tools.at(i);
							if (i == 6) pData->SurgItem7 = Tools.at(i);
							if (i == 7) pData->SurgItem8 = Tools.at(i);
							if (i == 8) pData->SurgItem9 = Tools.at(i);
							if (i == 9) pData->SurgItem10 = Tools.at(i);
							if (i == 10) pData->SurgItem11 = Tools.at(i);
							if (i == 11) pData->SurgItem12 = Tools.at(i);
							if (i == 12) pData->SurgItem13 = Tools.at(i);
						}

						vector<int> VisiTools{ 1258, 1260, 1262, 1264, 1266, 1268, 1270, 4308, 4310, 4312, 4314, 4316, 4318 };

						int TuriKartu = 1;
						bool Taip = false;
						for (int isd = 0; isd < VisiTools.size(); isd++)
						{
							bool Pirmas = false;
							SearchInventoryItem(peer, VisiTools[isd], 20, Pirmas);
							if (Pirmas)
							{
								continue;
							}
							bool Antras = false;
							SearchInventoryItem(peer, VisiTools[isd], 20, Antras);
							if (!Antras) {
								int arrayd = VisiTools.size() - TuriKartu;
								auto KiekTuri = 0;
								for (auto i = 0; i < pData->inventory.items.size(); i++)
								{
									if (pData->inventory.items.at(i).itemID == VisiTools[isd] && pData->inventory.items.at(i).itemCount >= 1)
									{
										KiekTuri = pData->inventory.items.at(i).itemCount;
									}
								}
								if (!Taip) surgerywarning += "You only have `4" + to_string(KiekTuri) + "`` " + getItemDef(VisiTools[isd]).name + ", ";
								else if (isd == arrayd)  surgerywarning += "and `4" + to_string(KiekTuri) + "`` " + getItemDef(VisiTools[isd]).name + " ";
								else surgerywarning += "`4" + to_string(KiekTuri) + "`` " + getItemDef(VisiTools[isd]).name + ", ";
								Taip = true;
							}
						}
						pData->RequestedSurgery = true;
						string LowSupplyWarning = "";
						if (surgerywarning != "") LowSupplyWarning = "\nadd_smalltext|`9Low Supply Warning: ``" + surgerywarning + "``|left|";
						Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`9Surg-E Anatomical Dummy``|left|4296|\nadd_spacer|small|\nadd_smalltext|Surgeon Skill: " + to_string(pData->SurgerySkill) + "|left|\nadd_textbox|Are you sure you want to perform surgery on this robot? Whether you succeed or fail, the robot will be destroyed in the process.|left|" + LowSupplyWarning + "\nend_dialog|surge|Cancel|Okay!|");
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 6016) {
					pData->lastPunchX = x;
					pData->lastPunchY = y;
					SendGScan(peer, world, x, y);
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 3898)
				{
					if (isWorldOwner(peer, world) || world->owner == "" || isDev(peer) || isWorldAdmin(peer, world)) {
						Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wTelefon`|left|3898|\n\nadd_spacer|small|\nadd_label|small|`ogrowplay'de birini aramak icin numara cevirin. Numara minumum 5 haneli olmalidir, 12345 gibi (deneyebilirsin - umarim cilgin olmazsin!). Cogu numara servis disidir!!|\nadd_spacer|small|\nadd_text_input|telephonenumber|Telefon #||5|\nend_dialog|usetelephone|Kapat|`wAra|\n");
					}
					return;
				}
				// LOCK-BOT
				if (world->items[x + (y * world->width)].foreground == 3682) {
					if (tile == 32 && tile != 18) {
						auto worldlocks = 0;
						auto diamondlocks = 0;
						auto locks = 0;
						for (auto i = 0; i < ((PlayerInfo*)(peer->data))->inventory.items.size(); i++) {
							if (((PlayerInfo*)(peer->data))->inventory.items.at(i).itemID == 242) {
								worldlocks = ((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount;
							}
							if (((PlayerInfo*)(peer->data))->inventory.items.at(i).itemID == 1796) {
								diamondlocks = ((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount;
							}
						}
						if (diamondlocks > 0) {
							locks = diamondlocks * 100;
						}
						locks += worldlocks;
						Player::OnDialogRequest(peer, "set_default_color|\nadd_label_with_icon|big|`9Lock-Bot|left|3682|\nadd_smalltext|`oGreetings human. I am Lock-Bot. I can provide convenient access to Locke's supply or goods, in exchange for Locks. However. I must leave to recharge soon.|left|\nadd_spacer|small|\nadd_smalltext|`9(Sensors detect " + to_string(locks) + " World Locks)|left|\nadd_spacer|small|\nadd_button|gtoken2|`oBuy Growtoken For 100 World Locks|0|0|\nend_dialog|Cl0se|Goodbye!||");
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 1436)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName == getPlyersWorld(peer)->owner || isMod(peer))
					{
						string line;
						GTDialog allLog;
						ifstream breaklog("save/securitycam/logs/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".txt");
						allLog.addLabelWithIcon("`wBreak Block Logs", 1434, LABEL_SMALL);
						allLog.addSpacer(SPACER_SMALL);
						for (string line; getline(breaklog, line);) {
							if (line.length() > 3 && line.at(0) != '/' && line.at(1) != '/') {
								allLog.addTextBox(line);
							}
						}
						breaklog.close();

						if (isDev(peer))
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wSecurity Camera``|left|1436||\nadd_textbox|`oThis camera is suitable for logging actions which was made by other peoples such as `#Moderators `oand `6Developers`o!|" + allLog.finishDialog() + "|\nadd_spacer|small|\nadd_button|clearworldlogs|`4Clear Logs|0|0|\nadd_button|asfasfasd|`wClose|0|0|\nadd_quick_exit|");
						}
						else if (isMod(peer))
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wSecurity Camera``|left|1436||\nadd_textbox|`oThis camera is suitable for logging actions which was made by other peoples such as `#Moderators `oand `6Developers`o!|" + allLog.finishDialog() + "|\nadd_spacer|small|\nadd_button|asfasfasd|`wClose|0|0|\nadd_quick_exit|");
						}
						else
						{
							Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wSecurity Camera``|left|1436||\nadd_textbox|`oThis camera is suitable for logging actions which was made by other peoples such as `#Moderators `oand `6Developers`o!|" + allLog.finishDialog() + "|\nadd_spacer|small|\nadd_button|clearworldlogs|`4Clear Logs|0|0|\nadd_button|asfasfasd|`wClose|0|0|\nadd_quick_exit|");
						}
					}
					else
					{
						Player::OnTextOverlay(peer, "`#Only world owner can view world logs!");
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 658)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the bulletin board.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myLetterBox;
							myLetterBox.addLabelWithIcon("`wBulletin Board", 658, LABEL_BIG);
							myLetterBox.addSpacer(SPACER_SMALL);
							try
							{
								ifstream ifff("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								json j;
								ifff >> j;
								ifff.close();
								if (j["inmail"] <= 0)
								{
									myLetterBox.addTextBox("`oThe Bulletin Board is empty.");
									myLetterBox.addSpacer(SPACER_SMALL);
								}
								else
								{
									for (int i = 0; i < 20; i++)
									{
										if (j["mails"].at(i)["growid"] != "")
										{
											if (j["hidenames"] == 0)
											{
												int apos = j["mails"].at(i)["aposition"];
												myLetterBox.addLabelWithIconButton("`w" + j["mails"].at(i)["growid"].get<string>() + ":`2 " + j["mails"].at(i)["text"].get<string>() + "", 660, "removeselectedbulletin_" + to_string(squaresign) + "_" + to_string(apos));
												//myLetterBox.addSpacer(SPACER_SMALL);
											}
											else
											{
												myLetterBox.addTextBox("`2" + j["mails"].at(i)["text"].get<string>());
												//myLetterBox.addSpacer(SPACER_SMALL);
											}
										}
									}
								}

								if (j["inmail"] < 90)
								{
									myLetterBox.addTextBox("`oAdd to conversation?");
									myLetterBox.addInputBox("addbulletinletterinput", "", "", 50);
									myLetterBox.addSpacer(SPACER_SMALL);
									myLetterBox.addButton("addbulletinletter_" + to_string(squaresign), "`2Add");
									myLetterBox.addSpacer(SPACER_SMALL);
								}

								myLetterBox.addLabelWithIcon("`wOwner Options", 242, LABEL_BIG);
								myLetterBox.addSpacer(SPACER_SMALL);
								if (j["hidenames"] == 1)
								{
									myLetterBox.addTextBox("`oUncheck `5Hide names `oto enable individual comment removal options.");
									myLetterBox.addSpacer(SPACER_SMALL);
								}
								else
								{
									myLetterBox.addTextBox("`oTo remove an individual comment, press the icon to the left of it.");
									myLetterBox.addSpacer(SPACER_SMALL);
								}
								if (j["inmail"] > 0)
								{
									myLetterBox.addButton("bulletinboardclear_" + to_string(squaresign), "`4Clear Board");
									myLetterBox.addSpacer(SPACER_SMALL);
								}
								if (j["publiccanadd"] == 1)
									myLetterBox.addCheckbox("publiccanaddbulletinboard", "`oPublic can add", CHECKBOX_SELECTED);
								else
									myLetterBox.addCheckbox("publiccanaddbulletinboard", "`oPublic can add", CHECKBOX_NOT_SELECTED);

								if (j["hidenames"] == 1)
									myLetterBox.addCheckbox("hidenamesbulletinboard", "`oHide names", CHECKBOX_SELECTED);
								else
									myLetterBox.addCheckbox("hidenamesbulletinboard", "`oHide names", CHECKBOX_NOT_SELECTED);
								myLetterBox.addSpacer(SPACER_SMALL);
								myLetterBox.addButton("bulletinletterok_" + to_string(squaresign), "`wOK");
								myLetterBox.addQuickExit();
								myLetterBox.endDialog("Close", "", "Cancel");
								Player::OnDialogRequest(peer, myLetterBox.finishDialog());
							}
							catch (std::exception&)
							{
								cout << "bulletin tile failed" << endl;
								return; /*tipo jeigu nera*/
							}
						}
						else
						{
							GTDialog myLetterBox;
							myLetterBox.addLabelWithIcon("`wBulletin Board", 658, LABEL_BIG);
							myLetterBox.addSpacer(SPACER_SMALL);
							try
							{
								ifstream ifff("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								json j;
								ifff >> j;
								ifff.close();

								if (j["inmail"] > 0)
								{
									for (int i = 0; i < 20; i++)
									{
										if (j["mails"].at(i)["growid"] != "")
										{
											if (j["hidenames"] == 0)
											{
												myLetterBox.addLabelWithIcon("`w" + j["mails"].at(i)["growid"].get<string>() + ":`2 " + j["mails"].at(i)["text"].get<string>() + "", 660, LABEL_SMALL);
												myLetterBox.addSpacer(SPACER_SMALL);
											}
											else
											{
												myLetterBox.addTextBox("`2" + j["mails"].at(i)["text"].get<string>());
												myLetterBox.addSpacer(SPACER_SMALL);
											}
										}
									}
								}

								if (j["publiccanadd"] == 1 && j["inmail"] < 90)
								{
									myLetterBox.addSpacer(SPACER_SMALL);
									myLetterBox.addTextBox("`oAdd to conversation?");
									myLetterBox.addInputBox("addbulletinletterinput", "", "", 50);
									myLetterBox.addSpacer(SPACER_SMALL);
									myLetterBox.addButton("addbulletinletter_" + to_string(squaresign), "`2Add");
									myLetterBox.addSpacer(SPACER_SMALL);
								}

								myLetterBox.addQuickExit();
								myLetterBox.endDialog("Close", "", "Cancel");
								Player::OnDialogRequest(peer, myLetterBox.finishDialog());
							}
							catch (std::exception&)
							{
								cout << "bulletin tile failed" << endl;
								return; /*tipo jeigu nera*/
							}
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 6286)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl1/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the box.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myBox;
							myBox.addLabelWithIcon("`wStorage Box Xtreme - Level 1", 6286, LABEL_BIG);
							ifstream ifff("save/storageboxlvl1/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							int stored = j["instorage"];

							if (stored > 0)
							{
								myBox.addSpacer(SPACER_SMALL);
							}

							int count = 0;
							int id = 0;
							int aposition = 0;
							for (int i = 0; i < 20; i++)
							{
								if (j["storage"].at(i)["itemid"] != 0)
								{
									count = j["storage"].at(i)["itemcount"];
									id = j["storage"].at(i)["itemid"];
									aposition = j["storage"].at(i)["aposition"];

									if (i % 6 == 0 && i != 0)
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl1DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), true);
									}
									else
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl1DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), false);
									}
								}
							}

							if (stored > 0)
							{
								myBox.addNewLineAfterFrame();
							}

							myBox.addTextBox("`w" + to_string(stored) + "/20 `$items stored.");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addPicker("boxlvl1deposit_" + to_string(squaresign), "Deposit item", "Select an item");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addQuickExit();
							myBox.endDialog("Close", "", "Exit");
							Player::OnDialogRequest(peer, myBox.finishDialog());
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 6288)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl2/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the box.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myBox;
							myBox.addLabelWithIcon("`wStorage Box Xtreme - Level 2", 6288, LABEL_BIG);
							ifstream ifff("save/storageboxlvl2/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							int stored = j["instorage"];

							if (stored > 0)
							{
								myBox.addSpacer(SPACER_SMALL);
							}

							int count = 0;
							int id = 0;
							int aposition = 0;
							for (int i = 0; i < 40; i++)
							{
								if (j["storage"].at(i)["itemid"] != 0)
								{
									count = j["storage"].at(i)["itemcount"];
									id = j["storage"].at(i)["itemid"];
									aposition = j["storage"].at(i)["aposition"];

									if (i % 6 == 0 && i != 0)
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl2DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), true);
									}
									else
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl2DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), false);
									}
								}
							}

							if (stored > 0)
							{
								myBox.addNewLineAfterFrame();
							}

							myBox.addTextBox("`w" + to_string(stored) + "/40 `$items stored.");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addPicker("boxlvl2deposit_" + to_string(squaresign), "Deposit item", "Select an item");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addQuickExit();
							myBox.endDialog("Close", "", "Exit");
							Player::OnDialogRequest(peer, myBox.finishDialog());
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 6290)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl3/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the box.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myBox;
							myBox.addLabelWithIcon("`wStorage Box Xtreme - Level 3", 6290, LABEL_BIG);
							ifstream ifff("save/storageboxlvl3/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							int stored = j["instorage"];

							if (stored > 0)
							{
								myBox.addSpacer(SPACER_SMALL);
							}

							int count = 0;
							int id = 0;
							int aposition = 0;
							for (int i = 0; i < 90; i++)
							{
								if (j["storage"].at(i)["itemid"] != 0)
								{
									count = j["storage"].at(i)["itemcount"];
									id = j["storage"].at(i)["itemid"];
									aposition = j["storage"].at(i)["aposition"];

									if (i % 6 == 0 && i != 0)
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl3DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), true);
									}
									else
									{
										myBox.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "boxlvl3DepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), false);
									}
								}
							}

							if (stored > 0)
							{
								myBox.addNewLineAfterFrame();
							}

							myBox.addTextBox("`w" + to_string(stored) + "/90 `$items stored.");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addPicker("boxlvl3deposit_" + to_string(squaresign), "Deposit item", "Select an item");
							myBox.addSpacer(SPACER_SMALL);
							myBox.addQuickExit();
							myBox.endDialog("Close", "", "Exit");
							Player::OnDialogRequest(peer, myBox.finishDialog());
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 8878)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/safevault/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4Safe Vault Erisime Kapatilmistir.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							ifstream ifff("save/safevault/_" + pData->currentWorld + "/X" + to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							int stored = j["insafe"];
							string password = j["password"];

							if (password != "")
							{
								GTDialog mySafeConfirm;
								mySafeConfirm.addLabelWithIcon("`wSafe Vault", 8878, LABEL_BIG);
								mySafeConfirm.addTextBox("Please enter your password to access the Save Vault.");
								mySafeConfirm.addInputBox("safeconfirmpassInput_" + to_string(squaresign), "", "", 18);
								mySafeConfirm.addButton("safe_confirmpass", "Enter Password");
								mySafeConfirm.addButton("saferecoverPasswordInConfirm_" + to_string(squaresign), "Recover Password");
								mySafeConfirm.addSpacer(SPACER_SMALL);
								mySafeConfirm.addQuickExit();
								mySafeConfirm.endDialog("Close", "", "Exit");
								Player::OnDialogRequest(peer, mySafeConfirm.finishDialog());
								return;
							}

							GTDialog mySafe;
							mySafe.addLabelWithIcon("`wSafe Vault", 8878, LABEL_BIG);

							if (stored > 0)
							{
								mySafe.addSpacer(SPACER_SMALL);
							}

							int count = 0;
							int id = 0;
							int aposition = 0;
							for (int i = 0; i < 20; i++)
							{
								if (j["safe"].at(i)["itemid"] != 0)
								{
									count = j["safe"].at(i)["itemcount"];
									id = j["safe"].at(i)["itemid"];
									aposition = j["safe"].at(i)["aposition"];

									if (i % 3 == 0 && i != 0)
									{
										mySafe.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "safeBoxDepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), true);
									}
									else
									{
										mySafe.addStaticBlueFrameWithIdCountText(to_string(id), to_string(count), getItemDef(id).name, "safeBoxDepositedItem_" + to_string(aposition) + "_" + to_string(squaresign), false);
									}
								}
							}

							if (stored > 0)
							{
								mySafe.addNewLineAfterFrame();
							}

							mySafe.addTextBox("`w" + to_string(stored) + "/20 `$items stored.");
							mySafe.addSpacer(SPACER_SMALL);
							mySafe.addPicker("safedeposit_" + to_string(squaresign), "Deposit item", "Select an item");
							if (j["password"] == "")
							{
								mySafe.addTextBox("`$This Safe Vault is not `4password protected`$!");
							}
							else
							{
								mySafe.addTextBox("`$This Safe Vault is `2password protected`$!");
							}
							mySafe.addSpacer(SPACER_SMALL);
							mySafe.addTextBox("`$Change your password.");
							mySafe.addButton("safeupdatepass_" + to_string(squaresign), "Update Password");

							mySafe.addSpacer(SPACER_SMALL);
							mySafe.addQuickExit();
							mySafe.endDialog("Close", "", "Exit");
							Player::OnDialogRequest(peer, mySafe.finishDialog());
						}
					}
					return;
				}
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DONATION)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the donation box.", 0, true);
					}
					else
					{
						pData->lastPunchX = x;
						pData->lastPunchY = y;
						pData->lastPunchForeground = world->items.at(x + (y * world->width)).foreground;
						pData->lastPunchBackground = world->items.at(x + (y * world->width)).background;
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myDbox;
							myDbox.addLabelWithIcon("`wDonation Box", world->items.at(x + (y * world->width)).foreground, LABEL_BIG);
							ifstream ifff("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();
							if (j["donated"] <= 0)
							{
								myDbox.addTextBox("`$The box is currently empty.");
							}
							else
							{
								int donated = j["donated"];
								int count = 0;
								myDbox.addTextBox("`oYou have `w" + to_string(donated) + " `ogifts waiting:");
								myDbox.addSpacer(SPACER_SMALL);
								for (int i = 0; i < 20; i++)
								{
									if (j["donatedItems"].at(i)["itemid"] != 0)
									{
										count = j["donatedItems"].at(i)["itemcount"];
										myDbox.addLabelWithIcon("`o" + getItemDef(j["donatedItems"].at(i)["itemid"]).name + " (`w" + to_string(count) + "`o) from `w" + j["donatedItems"].at(i)["sentBy"].get<string>() + "`#- '" + j["donatedItems"].at(i)["note"].get<string>() + "'", j["donatedItems"].at(i)["itemid"], LABEL_SMALL);
										myDbox.addSpacer(SPACER_SMALL);
									}
								}
								myDbox.addSpacer(SPACER_SMALL);
								myDbox.addButton("retrieveGifts_" + to_string(squaresign), "`4Retrieve Gifts");
							}
							myDbox.addSpacer(SPACER_SMALL);
							myDbox.addPicker("addDonationItem_" + to_string(squaresign), "`wGive Gift `o(Min rarity: `52`o)", "Select an item");
							myDbox.addSpacer(SPACER_SMALL);
							myDbox.addQuickExit();
							myDbox.endDialog("Close", "", "Cancel");
							Player::OnDialogRequest(peer, myDbox.finishDialog());
						}
						else
						{
							GTDialog myDbox;
							myDbox.addLabelWithIcon("`wDonation Box", world->items.at(x + (y * world->width)).foreground, LABEL_BIG);
							ifstream ifff("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							int donated = j["donated"];

							myDbox.addTextBox("`$You see `w" + to_string(donated) + "`$ gifts in the box!");
							myDbox.addTextBox("`$Want to leave a gift for the owner?");
							myDbox.addSpacer(SPACER_SMALL);

							myDbox.addPicker("addDonationItem_" + to_string(squaresign), "`wGive Gift `o(Min rarity: `52`o)", "Select an item");

							myDbox.addSpacer(SPACER_SMALL);
							myDbox.addQuickExit();
							myDbox.endDialog("Close", "", "Cancel");
							Player::OnDialogRequest(peer, myDbox.finishDialog());
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 1006)
				{
					auto squaresign = x + (y * world->width);
					auto isdbox = std::experimental::filesystem::exists("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					if (!isdbox)
					{
						Player::OnTalkBubble(peer, pData->netID, "`4An error occured. Break the mailbox.", 0, true);
					}
					else
					{
						if (pData->rawName == PlayerDB::getProperName(world->owner) || world->owner == "" || isDev(peer))
						{
							GTDialog myLetterBox;
							myLetterBox.addLabelWithIcon("`wBlue Mail Box", 1006, LABEL_BIG);
							ifstream ifff("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();
							if (j["inmail"] <= 0)
							{
								myLetterBox.addTextBox("`oThe mailbox is currently empty.");
							}
							else
							{
								int donated = j["inmail"];

								myLetterBox.addTextBox("`oYou have `w" + to_string(donated) + " `oletters:");
								myLetterBox.addSpacer(SPACER_SMALL);
								for (int i = 0; i < 20; i++)
								{
									if (j["mails"].at(i)["growid"] != "")
									{
										myLetterBox.addLabelWithIcon("`5\"" + j["mails"].at(i)["text"].get<string>() + "\" - `w" + j["mails"].at(i)["growid"].get<string>() + "", 660, LABEL_SMALL);
										myLetterBox.addSpacer(SPACER_SMALL);
									}
								}
								myLetterBox.addSpacer(SPACER_SMALL);
								myLetterBox.addButton("bluemailempty_" + to_string(squaresign), "`4Empty mailbox");
							}
							myLetterBox.addTextBox("`oWrite a letter to yourself?");
							myLetterBox.addInputBox("addblueletterinput_" + to_string(squaresign), "", "", 50);
							myLetterBox.addSpacer(SPACER_SMALL);
							myLetterBox.addButton("addblueletter", "`2Send Letter");
							myLetterBox.addSpacer(SPACER_SMALL);
							myLetterBox.addQuickExit();
							myLetterBox.endDialog("Close", "", "Cancel");
							Player::OnDialogRequest(peer, myLetterBox.finishDialog());
						}
						else
						{
							GTDialog myLetterBox;
							myLetterBox.addLabelWithIcon("`wMail Box", 1006, LABEL_BIG);
							ifstream ifff("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							json j;
							ifff >> j;
							ifff.close();

							myLetterBox.addTextBox("`$Want to leave a message for the owner?");
							myLetterBox.addSpacer(SPACER_SMALL);
							myLetterBox.addInputBox("addblueletterinput_" + to_string(squaresign), "", "", 50);
							myLetterBox.addSpacer(SPACER_SMALL);
							myLetterBox.addButton("addblueletter", "`2Send Letter");

							myLetterBox.addSpacer(SPACER_SMALL);
							myLetterBox.addQuickExit();
							myLetterBox.endDialog("Close", "", "Cancel");
							Player::OnDialogRequest(peer, myLetterBox.finishDialog());
						}
					}
					return;
				}
				if (world->items.at(x + (y * world->width)).foreground == 2946)
				{
					if (pData->rawName == world->owner || world->owner == "" || world->isPublic || isDev(peer))
					{
						int itemid = world->items.at(x + (y * world->width)).foreground;
						int itembg = world->items.at(x + (y * world->width)).background;
						pData->displayfg = itemid;
						pData->displaybg = itembg;
						pData->displaypunchx = data.punchX;
						pData->displaypunchy = data.punchY;
						if (world->items.at(x + (y * world->width)).intdata != 0 && pData->rawName == world->owner)
						{
							Player::OnDialogRequest(peer, "add_label_with_icon|big|`wDisplay Block|left|" + to_string(itemid) + "|\nadd_spacer|small||\nadd_label|small|`oA " + getItemDef(world->items.at(x + (y * world->width)).intdata).name + " is on display here.|\nadd_button|pickupdisplayitem|Pick it up|0|0|\nadd_quick_exit|\n");
						}
						else if (world->items.at(x + (y * world->width)).intdata != 0 && (isDev(peer)))
						{
							Player::OnDialogRequest(peer, "add_label_with_icon|big|`wDisplay Block|left|" + to_string(itemid) + "|\nadd_spacer|small||\nadd_label|small|`oA " + getItemDef(world->items.at(x + (y * world->width)).intdata).name + " is on display here.|\nadd_button|chc000|Okay|0|0|\nadd_quick_exit|\n");
						}
						else if (world->isPublic && world->items.at(x + (y * world->width)).intdata != 0 && pData->rawName != world->owner)
						{
							Player::OnDialogRequest(peer, "add_label_with_icon|big|`wDisplay Block|left|" + to_string(itemid) + "|\nadd_spacer|small||\nadd_label|small|`oA " + getItemDef(world->items.at(x + (y * world->width)).intdata).name + " is on display here.|\nadd_button|chc000|Okay|0|0|\nadd_quick_exit|\n");
						}
						else if (world->owner == "" && world->items.at(x + (y * world->width)).intdata != 0)
						{
							Player::OnDialogRequest(peer, "add_label_with_icon|big|`wDisplay Block|left|" + to_string(itemid) + "|\nadd_spacer|small||\nadd_label|small|`oA " + getItemDef(world->items.at(x + (y * world->width)).intdata).name + " is on display here.|\nadd_button|pickupdisplayitem|Pick it up|0|0|\nadd_quick_exit|\n");
						}
						else
						{
							Player::OnDialogRequest(peer, "add_label_with_icon|big|`wDisplay Block|left|" + to_string(itemid) + "|\nadd_spacer|small||\nadd_label|small|`oThe Display Block is empty. Use an item on it to display the item!|\nend_dialog||Close||\n");
						}
						return;
					}
					else
					{
						Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
						return;
					}
					return;
				}
				return;
			}
		case 6336:
			{
				SendGrowpedia(peer);
				return;
			}
		case 1436: /*securitycamera*/
		{
			if (tile == 1436)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 1436)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wYou need to be world owner to place `$Security Camera`w!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wYou cant place more than one `$Security Camera`w!", 0, true);
					return;
				}
				break;
			}
		}
		case 226:
		{
			if (tile == 226)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 226)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Signal Jammer`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 1276:
		{
			if (tile == 1276)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 1276)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Punch Jammer`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 1278:
		{
			if (tile == 1278)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 1278)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Zombie Jammer`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 3750:
		{
			if (tile == 3750)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 3750)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Ghost Charm`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 3616:
		{
			if (tile == 3616)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 3616)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Guardian Pineapple`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 4884:
		{
			if (tile == 4884)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 4884)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Balloon Jammer`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 4758:
		{
			if (tile == 4758)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 4758)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Mini-Mod`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 4992:
		{
			if (tile == 4992)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 4992)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Antigravity Generator`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 2072:
		{
			if (tile == 2072)
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++)
				{
					if (world->items[i].foreground == 2072)
					{
						aryra = true;
					}
				}
				if (aryra == false)
				{
					if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis item can only be used in World-Locked worlds!", 0, true);
						return;
					}
				}
				else
				{
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThis world already has a `$Xenonite growplay`w somewhere on it, installing two would be dangerous", 0, true);
					return;
				}
				break;
			}
		}
		case 3240:
		{
			RemoveInventoryItem(3240, 1, peer, true);
			{
				static_cast<PlayerInfo*>(peer->data)->GeigerCooldown = false;
				static_cast<PlayerInfo*>(peer->data)->haveGeigerRadiation = false;
				Player::OnTalkBubble(peer, pData->netID, "`#YUM!", 0, true);
				Player::OnConsoleMessage(peer, "`oYou are not longer radiation!. (`$Irradiated `omod remove)``");
			}
			return;
		}
		case 8774:
		{
			WorldInfo* info = getPlyersWorld(peer);
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
					continue;
				if (isHere(peer, currentPeer))
				{
					static_cast<PlayerInfo*>(peer->data)->currentWorld == "CLASHPARKOUR";
					Player::PlayAudio(peer, "audio/race_start.wav", 0);
				}
			}
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer))
				{
					Player::OnConsoleMessage(currentPeer, "CP:_PL:0_OID:_CT:[W]_ `6<`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`6> `6I SHALL FACE THE FINALE PARKOUR!!!");
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`6I SHALL FACE THE FINALE PARKOUR!!!", 0, true);
				}

			}
			//handle_world(peer, static_cast<PlayerInfo*>(peer->data)->currentWorld);
			return;
		}
		case 2992:
		{
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer))
				{
					Player::OnConsoleMessage(currentPeer, "CP:_PL:0_OID:_CT:[W]_ `6<`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`6> `6I SHALL FACE THE WOLF!!!");
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`6I SHALL FACE THE WOLF!!!", 0, true);
					{
						Player::PlayAudio(peer, "audio/snd037.wav", 0);
					}
				}
			}
			return;
		}
		case 274: case 276: case 278: case 732: case 408: case 618:
		{
			int xx = data.punchX;
			int yy = data.punchY;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (tile == 274 && static_cast<PlayerInfo*>(currentPeer->data)->x / 32 == xx && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == yy)
				{
					SendTradeEffect(currentPeer, 274, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					SendTradeEffect(peer, 274, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					RemoveInventoryItem(274, 1, peer, true);
					static_cast<PlayerInfo*>(currentPeer->data)->isFrozen = true;
					static_cast<PlayerInfo*>(currentPeer->data)->skinColor = -119470;
					static_cast<PlayerInfo*>(currentPeer->data)->freezetime = (GetCurrentTimeInternalSeconds()) + (10);
					Player::PlayAudio(currentPeer, "audio/freeze.wav", 0);
					sendClothes(currentPeer);
					sendFrozenState(currentPeer);
					Player::OnConsoleMessage(currentPeer, "Freeze! (`$Frozen `omod added, `$30 secs `oleft)");
					return;
				}
				if (tile == 408 && static_cast<PlayerInfo*>(currentPeer->data)->x / 32 == xx && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == yy)
				{
					RemoveInventoryItem(408, 1, peer, true);
					SendTradeEffect(currentPeer, 408, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					SendTradeEffect(peer, 408, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					Player::OnAddNotification(currentPeer, "`wWarning from `4System`w: You've been `4duct-taped `wfor 1 minutes", "audio/hub_open.wav", "interface/atomic_button.rttex");
					Player::OnConsoleMessage(currentPeer, "`oDuct tape has covered your mouth! (`$Duct Tape`o mod added, `$1 minutes`o left)");
					Player::OnConsoleMessage(currentPeer, "`oWarning from `4System`o: You've been `4duct-taped `ofor 1 minutes");
					Player::PlayAudio(currentPeer, "audio/already_used.wav", 0);
					static_cast<PlayerInfo*>(currentPeer->data)->taped = true;
					static_cast<PlayerInfo*>(currentPeer->data)->isDuctaped = true;
					static_cast<PlayerInfo*>(currentPeer->data)->cantsay = true;
					static_cast<PlayerInfo*>(currentPeer->data)->lastMuted = (GetCurrentTimeInternalSeconds()) + (30);
					send_state(currentPeer);
					sendClothes(currentPeer);
					return;
				}
				if (tile == 276 && static_cast<PlayerInfo*>(currentPeer->data)->x / 32 == xx && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == yy)
				{
					SendTradeEffect(currentPeer, 276, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					SendTradeEffect(peer, 276, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					RemoveInventoryItem(276, 1, peer, true);
					playerRespawn(world, currentPeer, true);
					Player::OnKilled(currentPeer, static_cast<PlayerInfo*>(currentPeer->data)->netID);
					int hi = static_cast<PlayerInfo*>(currentPeer->data)->x;
					int hi2 = static_cast<PlayerInfo*>(currentPeer->data)->y;
					Player::OnParticleEffect(currentPeer, 152, hi, hi2, 0);
					Player::OnParticleEffect(currentPeer, 4, hi, hi2, 0);
					return;
				}
				if (tile == 732 && static_cast<PlayerInfo*>(currentPeer->data)->x / 32 == xx && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == yy)
				{
					SendTradeEffect(peer, 732, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					SendTradeEffect(currentPeer, 732, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					RemoveInventoryItem(732, 1, peer, true);
					Player::OnConsoleMessage(currentPeer, "`#** `$The Ancient Ones`` have `4banned`` `w" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " `#** `o(`4/rules `oto see the rules!)");
					ENetPeer* currentPeer2;
					time_t now = time(0);
					char* dt = ctime(&now);
					for (currentPeer2 = server->peers;
						currentPeer2 < &server->peers[server->peerCount];
						++currentPeer2)
					{
						if (currentPeer2->state != ENET_PEER_STATE_CONNECTED)
							continue;
						if (static_cast<PlayerInfo*>(currentPeer2->data)->haveGrowId == false) continue;
						if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(currentPeer2->data)->rawName)
						{
							Player::OnAddNotification(currentPeer2, "`wWarning from `4System`w: You've been `4BANNED `wfor 730 days", "audio/hub_open.wav", "interface/atomic_button.rttex");
							Player::OnConsoleMessage(currentPeer2, "`wWarning from `4System`w: You've been `4BANNED `wfor 730 days");
							enet_peer_disconnect_later(currentPeer2, 0);
						}
					}
					return;
				}
				if (tile == 278 && static_cast<PlayerInfo*>(currentPeer->data)->x / 32 == xx && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == yy)
				{
					SendTradeEffect(peer, 278, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					SendTradeEffect(currentPeer, 278, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
					RemoveInventoryItem(278, 1, peer, true);
					string cursename = static_cast<PlayerInfo*>(currentPeer->data)->rawName;
					Player::OnConsoleMessage(currentPeer, "`#** `$The Ancients `ohave used `#Curse `oon `w" + cursename + "`o! `#**");
					if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId == false) continue;
					Player::OnAddNotification(currentPeer, "`wWarning from `4System`w: You've been `4CURSED `wfor 10 minutes", "audio/hub_open.wav", "interface/atomic_button.rttex");
					Player::OnConsoleMessage(currentPeer, "`wWarning from `4System`w: You've been `4CURSED `wfor 10 minutes");
					static_cast<PlayerInfo*>(currentPeer->data)->isCursed = true;
					static_cast<PlayerInfo*>(currentPeer->data)->lastCursed = (GetCurrentTimeInternalSeconds() + (5 * 60));
					sendPlayerLeave(currentPeer);
					static_cast<PlayerInfo*>(peer->data)->currentWorld = "EXIT";
					sendWorldOffers(currentPeer);
				}
			}
			return;
		}
		case 10536:
		{
			if (tile == 10536)
			{
				vector<int> list{ 3114, 3398, 386, 4422, 364, 9340, 9342, 9332, 9334, 9336, 9338, 366, 2388, 7808, 7810, 4416, 7818, 7820, 5652, 7822, 7824, 5644, 390, 7826, 7830, 9324, 5658, 3396, 2384, 5660, 3400, 4418, 4412, 388, 3408, 1470, 3404, 3406, 2390, 5656, 5648, 2396, 384, 5664, 4424, 4400, 8944 };
				int itemid = list[rand() % list.size()];
				if (itemid == 8944)
				{
					int target = 5;

					if ((rand() % 10000) <= target) {}
					else itemid = 3114;
				}
				RemoveInventoryItem(10536, 1, peer, true);
				Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wYou received `2" + getItemDef(itemid).name + " `wfrom a Special Winter Wish.", 0, true);
				Player::OnConsoleMessage(peer, "`oYou received `2" + getItemDef(itemid).name + " `ofrom a Special Winter Wish.");
				bool success = true;
				SaveItemMoreTimes(itemid, 1, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " from ssw");
				return;
			}
		}
		case 228: case 5764: case 1778: /*spray*/
			{
				if (isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 228 || isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 5764 || isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 1778) {
					spray_tree(peer, world, x, y, tile);
				} else if (!isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 228 || !isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 5764 || !isSeed(world->items.at(x + (y * world->width)).foreground) && tile == 1778) {
					Player::OnTalkBubble(peer, pData->netID, "Use this on a growing tree to speed it's growth.", 0, true);
				}
				return;
			}
		case 764:
			{
				if (pData->isZombie == true) return;
				if (pData->canWalkInBlocks == true)
				{
					pData->canWalkInBlocks = false;
					pData->skinColor = 0x8295C3FF;
					send_state(peer);
				}
				sendSound(peer, "skel.wav");
				pData->isZombie = true;
				send_state(peer);
				RemoveInventoryItem(764, 1, peer, true);
				playerconfig(peer, 1150, 130, 0x14);
				return;
			}
		case 782:
			{
				if (pData->isZombie == false) return;
				pData->isZombie = false;
				send_state(peer);
				RemoveInventoryItem(782, 1, peer, true);
				playerconfig(peer, 1150, 300, 0x14);
				return;
			}
		case 3694:
			{
				world->items.at(x + (y * world->width)).vid = 255; //r rgb value
				world->items.at(x + (y * world->width)).vprice = 128; //g rgb value
				world->items.at(x + (y * world->width)).vcount = 64; //b rgb value
				break;
			}
		case 3832:
			{
				world->items.at(x + (y * world->width)).intdata = 2;
				world->items.at(x + (y * world->width)).mc = 50;
				world->items.at(x + (y * world->width)).rm = false;
				world->items.at(x + (y * world->width)).opened = false;
				world->items.at(x + (y * world->width)).activated = false;
				break;
			}
		case 5000:
			{
				world->items.at(x + (y * world->width)).intdata = 14;
				world->items.at(x + (y * world->width)).activated = false;
				break;
			}
		case 6286: case 10190: case 10552: case 6288: case 6290: case 6214: case 1420: case 658: case 1006: case 8878: case 1240: case 762: case 6016:
			{
				if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer) || isWorldAdmin(peer, world))
				{
					//if (world->name == "CON" || world->name == "PRN" || world->name == "AUX" || world->name == "NUL" || world->name == "COM1" || world->name == "COM2" || world->name == "COM3" || world->name == "COM4" || world->name == "COM5" || world->name == "COM6" || world->name == "COM7" || world->name == "COM8" || world->name == "COM9" || world->name == "LPT1" || world->name == "LPT2" || world->name == "LPT3" || world->name == "LPT4" || world->name == "LPT5" || world->name == "LPT6" || world->name == "LPT7" || world->name == "LPT8" || world->name == "LPT9") return;
					if (tile == 6016) {
						if (world->owner == "") {
							Player::OnTalkBubble(peer, pData->netID, "Bu item sadece kilitli olan dunyalarda koyulabilir!", 0, true);
							return;
						}
					}

					if (tile == 6286 || tile == 6288 || tile == 6290)
					{
						auto Space = 20;
						string Directory = "save/storageboxlvl1";
						if (tile == 6288)
						{
							Space = 40; 
							Directory = "save/storageboxlvl2";
						}
						else if (tile == 6290)
						{
							Space = 90;
							Directory = "save/storageboxlvl3";
						}
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory(Directory + "/_" + world->name) || !fs::exists(Directory + "/_" + world->name))
						{
							fs::create_directory(Directory + "/_" + world->name);
						}
						ofstream of(Directory + "/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						json j;
						j["instorage"] = 0;
						auto jArray = json::array();
						json jmid;
						for (auto i = 1; i <= Space; i++)
						{
							jmid["aposition"] = i;
							jmid["itemid"] = 0;
							jmid["placedby"] = pData->rawName;
							jmid["itemcount"] = 0;
							jArray.push_back(jmid);
						}
						j["storage"] = jArray;
						of << j << std::endl;
						of.close();
					}
					if (tile == 1240) {
						isHeartMonitor = true;
					}
					if (tile == 8878)
					{
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory("save/safevault/_" + world->name) || !fs::exists("save/safevault/_" + world->name))
						{
							fs::create_directory("save/safevault/_" + world->name);
						}
						ofstream of("save/safevault/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						json j;
						j["insafe"] = 0;
						j["password"] = "";
						j["recovery"] = "";
						json jArray = json::array();
						json jmid;
						for (int i = 1; i <= 20; i++)
						{
							jmid["aposition"] = i;
							jmid["itemid"] = 0;
							jmid["placedby"] = pData->rawName;
							jmid["itemcount"] = 0;
							jArray.push_back(jmid);
						}
						j["safe"] = jArray;
						of << j << std::endl;
						of.close();
					}
					if (getItemDef(tile).blockType == BlockTypes::SHELF) {
						if (isDev(peer) || isWorldAdmin(peer, world) || isWorldOwner(peer, world)) {
							namespace fs = std::experimental::filesystem;
							if (!fs::is_directory("save/dshelf/" + world->name) || !fs::exists("save/dshelf/" + world->name))
							{
								fs::create_directory("save/dshelf/" + world->name);
							}
							json j;
							auto seedexist = std::experimental::filesystem::exists("save/dshelf/" + ((PlayerInfo*)(peer->data))->currentWorld + "/X" + std::to_string(squaresign) + ".txt");
							if (!seedexist)
							{
								ofstream dshelfxx("save/dshelf/" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".txt");
								dshelfxx << "0" << endl;
								dshelfxx << "0" << endl;
								dshelfxx << "0" << endl;
								dshelfxx << "0" << endl;
								dshelfxx.close();
							}
						}
					}
					if (tile == 656)
					{
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory("save/mailbox/_" + world->name) || !fs::exists("save/mailbox/_" + world->name))
						{
							fs::create_directory("save/mailbox/_" + world->name);
						}
						ofstream of("save/mailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						json j;
						j["x"] = x;
						j["y"] = y;
						j["inmail"] = 0;
						json jArray = json::array();
						json jmid;
						for (int i = 1; i <= 90; i++)
						{
							jmid["aposition"] = i;
							jmid["growid"] = "";
							jmid["text"] = "";
							jArray.push_back(jmid);
						}
						j["mails"] = jArray;
						of << j << std::endl;
						of.close();
					}
					if (tile == 658)
					{
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory("save/bulletinboard/_" + world->name) || !fs::exists("save/bulletinboard/_" + world->name))
						{
							fs::create_directory("save/bulletinboard/_" + world->name);
						}
						ofstream of("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						json j;
						j["inmail"] = 0;
						j["publiccanadd"] = 1;
						j["hidenames"] = 0;
						json jArray = json::array();
						json jmid;
						for (int i = 1; i <= 90; i++)
						{
							jmid["aposition"] = i;
							jmid["growid"] = "";
							jmid["text"] = "";
							jArray.push_back(jmid);
						}
						j["mails"] = jArray;
						of << j << std::endl;
						of.close();
					}
					if (tile == 1006)
					{
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory("save/bluemailbox/_" + world->name) || !fs::exists("save/bluemailbox/_" + world->name))
						{
							fs::create_directory("save/bluemailbox/_" + world->name);
						}
						ofstream of("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						json j;
						j["x"] = x;
						j["y"] = y;
						j["inmail"] = 0;
						json jArray = json::array();
						json jmid;
						for (int i = 1; i <= 90; i++)
						{
							jmid["aposition"] = i;
							jmid["growid"] = "";
							jmid["text"] = "";
							jArray.push_back(jmid);
						}
						j["mails"] = jArray;

						of << j << std::endl;
						of.close();
					}
					if (tile == 6214 || tile == 1420)
					{
						isMannequin = true;
						namespace fs = std::experimental::filesystem;
						if (!fs::is_directory("save/mannequin/_" + world->name) || !fs::exists("save/mannequin/_" + world->name))
						{
							fs::create_directory("save/mannequin/_" + world->name);
						}
						json j;
						auto seedexist = std::experimental::filesystem::exists("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
						if (!seedexist)
						{
							j["clothHead"] = "0";
							j["clothHair"] = "0";
							j["clothMask"] = "0";
							j["clothNeck"] = "0";
							j["clothBack"] = "0";
							j["clothShirt"] = "0";
							j["clothPants"] = "0";
							j["clothFeet"] = "0";
							j["clothHand"] = "0";
							ofstream of("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							of << j;
							of.close();
						}
					}
					break;
				}
			}
		case 2978: case 9268: /*vend*/
			{
				if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer) || isWorldAdmin(peer, world)) {
					VendUpdate = true;
					world->items.at(squaresign).vcount = 0;
					world->items.at(squaresign).vprice = 0;
					world->items.at(squaresign).vid = 0;
					world->items.at(squaresign).vdraw = 0;
					world->items.at(squaresign).opened = true;
					world->items.at(squaresign).rm = false;
					break;
				}
				else return;
			}
		case 2410: case 4426: case 1212: case 1234: case 3110: case 1976: case 2500: case 3122: case 10386: case 5664: case 5662: case 9644: case 5192: case 5194:
			{

				if (tile == 9644)
				{
					SearchInventoryItem(peer, 9644, 1, iscontains);
					if (!iscontains) return;
					else
					{
						if (CheckItemMaxed(peer, 1258, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1258).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1260, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1260).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1262, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1262).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1264, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1264).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1266, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1266).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1268, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1268).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 1270, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(1270).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4308, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4308).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4310, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4310).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4312, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4312).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4314, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4314).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4316, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4316).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4318, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4318).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}

						if (CheckItemMaxed(peer, 4296, 5))
						{
							Player::OnTalkBubble(peer, pData->netID, "" + getItemDef(4296).name + " wouldnt fit into my inventory!", 0, true);
							return;
						}


						RemoveInventoryItem(9644, 1, peer, true);
						Player::OnTalkBubble(peer, pData->netID, "`wYou received 5 Surgical Sponge, 5 Surgical Scalpel, 5 Surgical Anesthetic, 5 Surgical Antiseptic, 5 Surgical Antibiotics, 5 Surgical Splint, 1 Surgical Stitches, 5 Surgical Pins, 5 Surgical Transfusion, 5 Surgical Defibrillator, 5 Surgical Clamp, 5 Surgical Ultrasound, 5 Surgical Lab Kit and a 1 Surg-E", 0, true);
						Player::OnConsoleMessage(peer, "`oYou received 5 Surgical Sponge, 5 Surgical Scalpel, 5 Surgical Anesthetic, 5 Surgical Antiseptic, 5 Surgical Antibiotics, 5 Surgical Splint, 1 Surgical Stitches, 5 Surgical Pins, 5 Surgical Transfusion, 5 Surgical Defibrillator, 5 Surgical Clamp, 5 Surgical Ultrasound, 5 Surgical Lab Kit and a 1 Surg-E");
						bool success = true;
						SaveItemMoreTimes(1258, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1260, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1262, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1264, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1266, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1268, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(1270, 1, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4308, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4310, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4312, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4314, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4316, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4318, 5, peer, success, pData->rawName + " from surgery tool pack");
						SaveItemMoreTimes(4296, 1, peer, success, pData->rawName + " from surgery tool pack");

						ENetPeer* currentPeer;
						for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
						{
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer))
							{
								SendTradeEffect(currentPeer, 1258, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1260, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1262, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1264, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1266, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1268, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 1270, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4308, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4310, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4312, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4314, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4316, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4318, pData->netID, pData->netID, 150);
								SendTradeEffect(currentPeer, 4296, pData->netID, pData->netID, 150);

							}
						}


					}
				}

				if (tile == 5662)
				{
					SearchInventoryItem(peer, 5662, 250, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6You will need more dust than that!", 0, true);
					else
					{
						RemoveInventoryItem(5662, 250, peer, true);
						bool success = true;
						SaveItemMoreTimes(5642, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5The dust stirs and begins to swirl! Cupid appears before you.", 0, true);
						pData->cloth_hand = 5642;
						sendClothes(peer);
					}
				}

				if (tile == 5664)
				{
					SearchInventoryItem(peer, 5664, 1, iscontains);
					if (!iscontains) return;
					else
					{
						RemoveInventoryItem(5664, 1, peer, true);
						bool success = true;
						SaveItemMoreTimes(5662, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						//Player::OnConsoleMessage(peer, "`oYou received `21 " + getItemDef(itemid).name + " `ofrom the Gift of Growganoth.");
					}
				}
				if (tile == 10386)
				{
					SearchInventoryItem(peer, 10386, 1, iscontains);
					if (!iscontains) return;
					else
					{
						RemoveInventoryItem(10386, 1, peer, true);
						int itemuMas[59] = { 1216, 1218, 1992, 1982, 1994, 1972, 1980, 1988, 1984, 3116, 3102, 3106, 3110, 4160, 4162, 4164, 4154, 4158, 5224, 5222, 5232, 5240, 5248, 5238, 5256, 7116, 7108, 7110, 7128, 7112, 7114, 7130, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 1236, 1182, 1184, 1186, 1188, 1170, 10198, 10196, 10202, 10204, 10326, 10324, 10322 };
						auto randIndex = rand() % 59;
						int itemid = itemuMas[randIndex];
						bool success = true;
						SaveItemMoreTimes(itemid, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnConsoleMessage(peer, "`oYou received `21 " + getItemDef(itemid).name + " `ofrom the Gift of Growganoth.");
						Player::OnTalkBubble(peer, pData->netID, "`wYou received `21 " + getItemDef(itemid).name + " `wfrom the Gift of Growganoth.", 0, true);
					}
				}

				if (tile == 5192) {
					SearchInventoryItem(peer, 5192, 75, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough earth pigments here to make anything. Get more!", 0, true);
					else {
						RemoveInventoryItem(5192, 75, peer, true);
						bool success = true;
						SaveItemMoreTimes(7558, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The fragments forms into a earth wings!", 0, true);
						pData->cloth_back = 7558;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}

				if (tile == 5194) {
					SearchInventoryItem(peer, 5194, 75, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough lava pigments here to make anything. Get more!", 0, true);
					else {
						RemoveInventoryItem(5194, 75, peer, true);
						bool success = true;
						SaveItemMoreTimes(5196, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The fragments forms into a magma wings!", 0, true);
						pData->cloth_back = 5196;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}

				if (tile == 3122)
				{
					SearchInventoryItem(peer, 3122, 16, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough fragments here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(3122, 16, peer, true);
						bool success = true;
						SaveItemMoreTimes(3120, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The fragments forms into a teeny devil wings!", 0, true);
						pData->cloth_back = 3120;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}
				if (tile == 2500)
				{
					SearchInventoryItem(peer, 2500, 20, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough bones here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(2500, 20, peer, true);
						bool success = true;
						SaveItemMoreTimes(1998, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The bones forms into a skeletal dragon claw!", 0, true);
						pData->cloth_hand = 1998;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}
				if (tile == 1976)
				{
					SearchInventoryItem(peer, 1976, 10, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough skulls here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(1976, 10, peer, true);
						bool success = true;
						SaveItemMoreTimes(1974, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The skulls forms into a nightmare magnifying glass!", 0, true);
						pData->cloth_hand = 1974;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}
				if (tile == 1212)
				{
					SearchInventoryItem(peer, 1212, 25, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough fur here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(1212, 25, peer, true);
						bool success = true;
						SaveItemMoreTimes(1190, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The fur forms into a cuddly black cat!", 0, true);
						pData->cloth_hand = 1190;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}
				if (tile == 1234)
				{
					SearchInventoryItem(peer, 1234, 4, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough shards here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(1234, 4, peer, true);
						bool success = true;
						SaveItemMoreTimes(1206, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The shards forms into a devil wings!", 0, true);
						pData->cloth_back = 1206;
						Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
						sendClothes(peer);
					}
				}
				if (tile == 3110)
				{
					SearchInventoryItem(peer, 3110, 25, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`6There's just not enough tatters here to make anything. Get more!", 0, true);
					else
					{
						RemoveInventoryItem(3110, 25, peer, true);
						if ((rand() % 100) <= 15)
						{
							bool success = true;
							SaveItemMoreTimes(3112, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
							Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The tatters forms into a inside-out vampire cape!", 0, true);
							pData->cloth_back = 3112;
							Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
							sendClothes(peer);
						}
						else
						{
							bool success = true;
							SaveItemMoreTimes(1166, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
							Player::OnTalkBubble(peer, pData->netID, "`5SQUISH! The tatters forms into a vampire cape!", 0, true);
							pData->cloth_back = 1166;
							Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
							sendClothes(peer);
						}
					}
				}
				if (tile == 2410)
				{
					SearchInventoryItem(peer, 2410, 250, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`oYou will need more `^Emerald Shards `oFor that!", 0, true);
					else
					{
						Player::OnTalkBubble(peer, pData->netID, "`oThe power of `^Emerald Shards `oCompressed into `2Emerald Lock`o!", 0, true);
						RemoveInventoryItem(2410, 250, peer, true);
						bool success = true;
						SaveItemMoreTimes(2408, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnConsoleMessage(peer, "`o>> You received emerald lock!");
					}
				}
				if (tile == 4426)
				{
					SearchInventoryItem(peer, 4426, 250, iscontains);
					if (!iscontains) Player::OnTalkBubble(peer, pData->netID, "`oYou will need more `4Ruby Shards `oFor that!", 0, true);
					else
					{
						Player::OnTalkBubble(peer, pData->netID, "`oThe power of `4Ruby Shards `oCompressed into `4Ruby Lock`o!", 0, true);
						RemoveInventoryItem(4426, 250, peer, true);
						auto success = true;
						SaveItemMoreTimes(4428, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
						Player::OnConsoleMessage(peer, "`o>> You received ruby lock!");
					}
				}
				return;
			}
		case 6856: case 6858: case 6860: case 6862: case 9266: case 8186: case 8188: /*subtokens*/
			{
				int Sub_Days = 3;
				string Sub_Type = "free";
				if (tile == 9266) Sub_Days = 1;
				if (tile == 6856) Sub_Days = 3;
				if (tile == 6858) Sub_Days = 14;
				if (tile == 6860 || tile == 8186) {
					Sub_Days = 30; 
					Sub_Type = "premium";
				}
				if (tile == 6862 || tile == 8188) {
					Sub_Days = 365; 
					Sub_Type = "premium";
				}
				if (x == pData->x / 32 && y == pData->y / 32) {
					if (pData->Subscriber) {
						Player::OnTalkBubble(peer, pData->netID, "Zaten VIP ozelliklerin var!", 0, false);
						return;
					}
					pData->subtype = Sub_Type;
					pData->subdate = to_string(Sub_Days);
					RemoveInventoryItem(tile, 1, peer, true);
					SendTradeEffect(peer, tile, pData->netID, pData->netID, 150);
					pData->Subscriber = true;
					pData->haveSuperSupporterName = true;
					pData->SubscribtionEndDay = Sub_Days;
					Player::OnParticleEffect(peer, 46, pData->x, pData->y, 0);
					Player::OnAddNotification(peer, "`wTebrikler! `5VIP `$ozelliklerinin kilidini actin`w!", "audio/hub_open.wav", "interface/cash_icon_overlay.rttex");
					Player::PlayAudio(peer, "audio/thunderclap.wav", 0);
					try {
						ifstream read_player("save/players/_" + pData->rawName + ".json");
						if (!read_player.is_open()) {
							return;
						}		
						json j;
						read_player >> j;
						read_player.close();
						string title = j["title"];
						string chatcolor = j["chatcolor"];
						pData->NickPrefix = title;
						if (pData->NickPrefix != "") {
							restoreplayernick(peer);
							Player::OnNameChanged(peer, pData->netID, pData->NickPrefix + ". " + pData->tankIDName);
						}
						pData->chatcolor = chatcolor;
					} catch (std::exception& e) {
						std::cout << e.what() << std::endl;
						return;
					}
					send_state(peer);
					auto iscontains = false;
					SearchInventoryItem(peer, 6260, 1, iscontains);
					if (!iscontains) {
						bool success = true;
						SaveItemMoreTimes(6260, 1, peer, success, pData->rawName + " from subscription");
						Player::OnAddNotification(peer, "`wVIP SAYESINDE `5Amulet Of Force verildi`w!", "audio/hub_open.wav", "interface/cash_icon_overlay.rttex");
					}
				} else {
					Player::OnTalkBubble(peer, pData->netID, "Sadece kendi uzerinde kullanabilirsin.", 0, true); 
				}
				return;
			}
		case 196: case 9528: case 9530: case 7190: case 7480: case 5954: case 10456: case 9526: case 528: case 540: case 6918: case 6924: case 1662: case 3062: case 822: case 5706: case 9286: case 5750: case 4598: case 4596: case 7056: case 6316: case 4594: case 4604: /*consumables*/
			{
				if(tile == 9528) //handcuffs
				{
					if(static_cast<PlayerInfo*>(peer->data)->job != "police")
					{
						Player::OnTextOverlay(peer, "`oBu esyayi kullanabilmeniz icin Polis Sefi olmaniz gerekmektedir.");
						return;
					}
					if (static_cast<PlayerInfo*>(peer->data)->isarrestcooldown)
					{
						Player::OnTextOverlay(peer, "`o20 Dakikalik Bekleme Sureniz Var.");
						return;
					}
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (isHere(peer, currentPeer)) {
							int playerx = static_cast<PlayerInfo*>(currentPeer->data)->x;
							int playery = static_cast<PlayerInfo*>(currentPeer->data)->y;
							if (playerx / 32 == x && playery / 32 == y || playerx / 32 + 1 == x && playery / 32 == y || playerx / 32 + 2 == x && playery / 32 == y || playerx / 32 - 1 == x && playery / 32 == y || playerx / 32 - 2 == x && playery / 32 == y) {
								if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == pData->rawName) continue;
								if (static_cast<PlayerInfo*>(currentPeer->data)->wantedstars <=0)
								{
									Player::OnTextOverlay(peer, "`oOyuncu arananlar listesinde mevcut degil.");
									break;
								}
								if (static_cast<PlayerInfo*>(currentPeer->data)->isjailed)
								{
									Player::OnTextOverlay(peer, "`oBu oyuncu zaten tutuklanmis.");
									break;
								}
								if (static_cast<PlayerInfo*>(currentPeer->data)->isHospitalized)
								{
									Player::OnTextOverlay(peer, "`oOnu su anda tutuklayamazsiniz, cunku `2tedavi goruyor`o.");
									break;
								}
								if (static_cast<PlayerInfo*>(currentPeer->data)->isCursed)
								{
									Player::OnTextOverlay(peer, "`oOnu su anda tutuklayamazsiniz, cunku o `4cezali.");
									break;
								}
								RemoveInventoryItem(9528, 1, peer, true);
								int reward = 0;
								if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 1) reward = 25000;
								else if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 2) reward = 50000;
								else if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 3) reward = 75000;
								else if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 4) reward = 100000;
								else if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 5) reward = 150000;
								else if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars == 6) reward = 250000;
								else reward = 9999;
								Player::OnAddNotification(currentPeer, "`4Bir Polis Memuru tarafindan tutuklandiniz.","audio/startopia_tool_droid.wav","interface/large/game_over.rttex");
								Player::OnTextOverlay(peer, "`1"+static_cast<PlayerInfo*>(currentPeer->data)->displayName+" `obasari ile tutuklandi. Verilen odul `1"+to_string(reward)+" elmas.");
								std::ifstream ifsz("save/gemdb/_" + pData->rawName + ".txt");
								std::string content((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
								int gembux = atoi(content.c_str());
								int fingembux = gembux + reward;
								ofstream myfile;
								myfile.open("save/gemdb/_" + pData->rawName + ".txt");
								myfile << fingembux;
								myfile.close();
								int gemcalc = gembux + reward;
								Player::OnSetBux(peer, gemcalc, 0);
								static_cast<PlayerInfo*>(peer->data)->arrestcooldown = GetCurrentTimeInternalSeconds() + 1250;
								static_cast<PlayerInfo*>(peer->data)->isarrestcooldown = true;

								static_cast<PlayerInfo*>(currentPeer->data)->isjailed = true;
								static_cast<PlayerInfo*>(currentPeer->data)->jailedtime =  static_cast<PlayerInfo*>(currentPeer->data)->wantedstars;
								static_cast<PlayerInfo*>(currentPeer->data)->updateJailedTime = GetCurrentTimeInternalSeconds() + 240;
								static_cast<PlayerInfo*>(currentPeer->data)->wantedstars = 0;
								handle_world(currentPeer, "BASLA");
								
								for (ENetPeer* currentPeer9 = server->peers; currentPeer9 < &server->peers[server->peerCount]; ++currentPeer9) {
									if (currentPeer9->state != ENET_PEER_STATE_CONNECTED || currentPeer9->data == NULL) continue;
										Player::OnConsoleMessage(currentPeer9, static_cast<PlayerInfo*>(currentPeer->data)->displayName + " `7bir `1Polis `7memuru tarafından tutuklandı.");
								}
								
								break;
							}
						}
					}
				}
				if(tile == 9530) //marijuana
				{
					if (x == pData->x / 32 && y == pData->y / 32) {
						if(pData->food == 100)
						{
							Player::OnTalkBubble(peer, pData->netID, "`wYou're not hungry!", 0, true);
							return;
						}
						if(pData->job == "police")
						{
							Player::OnTalkBubble(peer, pData->netID, "`wYou cant use drugs, because you're a Police Officer!", 0, true);
							return;
						}
						RemoveInventoryItem(9530, 1, peer, true);
						pData->food = min(pData->food + 70, 100);
						pData->wantedstars = min(pData->wantedstars + 2, 6);
						Player::OnTextOverlay(peer, "`oHunger restored to `2"+to_string(pData->food)+"`o/`1100`o. You got `12 `1wanted stars `ofor using drugs.");
					}
					else
					{
						Player::OnTalkBubble(peer, pData->netID, "Must be used on a person.", 0, true); 
					}
				}
				if (tile == 5954)
				{
					RemoveInventoryItem(5954, 1, peer, true);
					vector<int> gitems{ 5818, 5820, 5822, 5824, 5826, 5828, 5830, 5832, 5834, 5854, 5874, 5894, 5914, 5946, 5944 };
					int rand_item = gitems[rand() % gitems.size()];
					Player::OnTalkBubble(peer, pData->netID, "`wYou received a `2" + getItemDef(rand_item).name + " `wfrom the Guild Chest.", 0, false);
					bool success = true;
					SaveItemMoreTimes(rand_item, 1, peer, success);
				}
				if (tile == 7190)
				{
					if (pData->guild == "")
					{
						Player::OnTalkBubble(peer, pData->netID, "Only a Guild Leader can use it.", 0, true);
						return;
					}
					GuildInfo* guild = guildDB.get_pointer(pData->guild);
					if (guild == NULL)
					{
						Player::OnConsoleMessage(peer, "`4An error occurred while getting guilds information! `1(#1)`4.");
						return;
					}
					GuildRanks myrank = GuildGetRank(guild, pData->rawName);
					if (myrank != GuildRanks::Leader)
					{
						Player::OnTalkBubble(peer, pData->netID, "Only a Guild Leader can use it.", 0, true);
						return;
					}
					GTDialog changeguildname;
					changeguildname.addCustom(GetGuildCustomDialogTextForGuildLabel(guild, "`wChange Guild Name``", "big"));
					changeguildname.addCustom("add_spacer|small|");
					changeguildname.addCustom("add_textbox|`oCurent Guild Name: "+guild->name+"|left|");
					changeguildname.addCustom("add_text_input|newguildname|`oGuild Name:||15|");
					changeguildname.addCustom("add_spacer|small|");
					changeguildname.addCustom("add_button|changeguildname|Confirm|noflags|0|0|");
					changeguildname.addQuickExit();
					changeguildname.endDialog("Close", "", "Exit");
					Player::OnDialogRequest(peer, changeguildname.finishDialog());
				}
				if (tile == 10456) { //mystic transform growplay
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						if (pData->cloth_back == 10456) {
							pData->cloth_back = 0;
							pData->noEyes = false;
							pData->noBody = false;
							pData->noHands = false;
							send_state(peer);
							sendClothes(peer);
						}
						else if (pData->cloth_back == 10426) {
							pData->cloth_back = 10456;
							pData->noEyes = true;
							pData->noBody = true;
							pData->noHands = true;
							send_state(peer);
							sendClothes(peer);
						}
					}
				}
				if (tile == 7480) { //Growformer PowerKey
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						if (pData->cloth_back == 7480) {
							pData->cloth_back = 0;
							pData->noEyes = false;
							pData->noBody = false;
							pData->noHands = false;
							send_state(peer);
							sendClothes(peer);
						}
						else if (pData->cloth_feet == 7384) {
							pData->cloth_back = 7480;
							pData->noEyes = true;
							pData->noBody = true;
							pData->noHands = true;
							send_state(peer);
							sendClothes(peer);
						}
					}
				}
				if(tile == 9526) //lock picks
				{
					if(world->isEvent) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									return;
								}
								if(world->name == "BASLA" || world->name == "JAIL" || world->name == "SEHIR" || world->name == "GAME1" || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
									return;

								if(pData->isrobbing) return;

								if (pData->rawName != whoslock) {
									if(pData->job != "robber")
									{
										Player::OnTextOverlay(peer, "`oYou need to get \"`1Robber`o\" `1job `o(`7wrench your-self - get a job`o).");
										return;
									}
									if (static_cast<PlayerInfo*>(peer->data)->isrobcooldown)
									{
										Player::OnTextOverlay(peer, "`oYou have 20 minutes cooldown.");
										return;
									}
									bool iscontainsss = false;
									if(world->items.at(x + (y * world->width)).foreground == 4802)
									{
										SearchInventoryItem(peer, 9526, 15, iscontainsss);
										if(!iscontainsss)
										{
											Player::OnTextOverlay(peer, "`oYou don't have `115 "+itemDefs[9526].name+" `oto rob `1"+itemDefs[4802].name+"ed `oworld.");
											return;
										}
										RemoveInventoryItem(9526, 15, peer, true);
									}
									else if(world->items.at(x + (y * world->width)).foreground == 7188)
									{
										SearchInventoryItem(peer, 9526, 10, iscontainsss);
										if(!iscontainsss)
										{
											Player::OnTextOverlay(peer, "`oYou don't have `110 "+itemDefs[9526].name+" `oto rob `1"+itemDefs[7188].name+"ed `oworld.");
											return;
										}
										RemoveInventoryItem(9526, 10, peer, true);
									}
									else if(world->items.at(x + (y * world->width)).foreground == 1796)
									{
										SearchInventoryItem(peer, 9526, 5, iscontainsss);
										if(!iscontainsss)
										{
											Player::OnTextOverlay(peer, "`oYou don't have `15 "+itemDefs[9526].name+" `oto rob `1"+itemDefs[1796].name+"ed `oworld.");
											return;
										}
										RemoveInventoryItem(9526, 5, peer, true);
									}
									else
									{
										SearchInventoryItem(peer, 9526, 1, iscontainsss);
										if(!iscontainsss)
										{
											Player::OnTextOverlay(peer, "`oYou don't have `11 "+itemDefs[9526].name+" `oto rob `1"+itemDefs[world->items.at(x + (y * world->width)).foreground].name+"ed `oworld.");
											return;
										}
										RemoveInventoryItem(9526, 1, peer, true);
									}
									if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner && world->owner != "")
									{
										bool islegitnow = false;
										for (int i = 0; i < world->width * world->height; i++)
										{
											if (world->items[i].foreground == 1436)
											{
												islegitnow = true;
												break;
											}
										}
										if (islegitnow == true)
										{
											string toLogs = "";
											toLogs = static_cast<PlayerInfo*>(peer->data)->displayName + " `w(" + static_cast<PlayerInfo*>(peer->data)->rawName + "`w) `5was `2robbing `5your world.";
											ofstream breaklogs("save/securitycam/logs/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".txt", ios::app);
											breaklogs << toLogs << endl;
											breaklogs.close();
										}
									}
									for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
										if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
										Player::OnConsoleMessage(currentPeer, pData->displayName+" `ois robbing `1"+world->name+"`o world now.");
									}
									if(static_cast<PlayerInfo*>(peer->data)->canWalkInBlocks) SendGhost(peer);
									Player::OnTextOverlay(peer, "`2Success! `1You have `210 seconds `1to rob this world.");
									Player::PlayAudio(peer, "audio/airy02.wav", 0);
									accessPlayerCustom(peer, world);

									if(pData->wantedstars == 0) pData->removewantedstar = GetCurrentTimeInternalSeconds() + 900;
									pData->wantedstars = min(pData->wantedstars + 3, 6);
									pData->lastrobbing = GetCurrentTimeInternalSeconds() + 11;
									pData->isrobbing = true;
									pData->robbingworld = world->name;

									static_cast<PlayerInfo*>(peer->data)->robcooldown = GetCurrentTimeInternalSeconds() + 1250;
									static_cast<PlayerInfo*>(peer->data)->isrobcooldown = true;

								}
							}
					}
				}
				if (tile == 5706) /*ssp*/
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (x == pData->x / 32 && y == pData->y / 32) {
						RemoveInventoryItem(5706, 1, peer, true);
						int Seed1 = 0;
						int Seed2 = 0;
						int Seed3 = 0;
						int Seed4 = 0;
						int Seed5 = 0;
						int AVGRarity = rand() % 9 + 1;
						while (Seed1 == 0 || Seed2 == 0 || Seed3 == 0 || Seed4 == 0 || Seed5 == 0) {
							for (int i = 0; i < maxItems; i++) {
								if (i >= 1000) {
									Player::OnTalkBubble(peer, pData->netID, "Something went wrong.", 0, true);
									break;
								}
								if (isSeed(i) && getItemDef(i).rarity == AVGRarity || isSeed(i) && getItemDef(i).rarity == AVGRarity + 1) {
									if (Seed1 == 0) Seed1 = i;
									else if (Seed2 == 0) Seed2 = i;
									else if (Seed3 == 0) Seed3 = i;
									else if (Seed4 == 0) Seed4 = i;
									else if (Seed5 == 0) Seed5 = i;
									else break;
									if (Seed4 != 0 && Seed5 == 0) AVGRarity = rand() % 9 + 1;
									else AVGRarity = rand() % 5 + 10;
								} else if (i == maxItems - 1) {
									break;
								}
							}
						}
						bool success = true;
						SaveItemMoreTimes(Seed1, 1, peer, success, "From small seed pack");
						SaveItemMoreTimes(Seed2, 1, peer, success, "From small seed pack");
						SaveItemMoreTimes(Seed3, 1, peer, success, "From small seed pack");
						SaveItemMoreTimes(Seed4, 1, peer, success, "From small seed pack");
						SaveItemMoreTimes(Seed5, 1, peer, success, "From small seed pack");
					} else {
						Player::OnTalkBubble(peer, pData->netID, "Must be used on a person.", 0, true); 
					}
				}
				if (tile == 9286 || tile == 5750) /*lucky fortune cookie*/
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (isHere(peer, currentPeer)) {
							if (x == pData->x / 32 && y == pData->y / 32) {
								RemoveInventoryItem(tile, 1, peer, true);
								vector<int> lunar_new_year{10616, 10582, 10580, 10664, 10596, 10598, 10586, 10590, 10592, 10576, 10578, 202, 204, 206, 4994, 2978, 5766, 5768, 5744, 5756, 5758, 5760, 5762, 5754, 7688, 7690, 7694, 7686, 7692, 7698, 7696, 9286, 9272, 9290, 9280, 9282, 9292, 9284};
								vector<string> lunar_messages{"`5Fortune: You will only get what you have the courage to pursue.``", "`5Fortune: You will live two lives.The second will begin when you realize you only have one.``", "`5Fortune: Looking on your past is fine. Same for the future. Just don't get caught staring.``", "`5Fortune: Your reality check is about to bounce.``", "`5Fortune: Telling people their future is easy. The hard part is being right.``", "`5Fortune: Deal with the faults of others as gently as your own.``", "`5Fortune: Things could always be better. The key is knowing if they're good enough.``", "`5Fortune: You will live two lives. The second will begin when you realize you only have one.``", "`5Fortune: Things could always be better. The key is knowing if they're good enough.``", "`5Fortune: Knowledge is worthless unless put into practice.``", "`5Fortune: You can't stop the waves, but you can learn to surf.``", "`5Fortune: A block in the hand is worth two on the tree.``", "`5Fortune: If it feels like you're always arguing with idiots, consider the company you keep.``", "`5Fortune: Don't chase happiness - create it.``", "`5Fortune: Even a fish wouldn't get into trouble if it kept its mouth shut.``", "`5Fortune: One's measure is in how they treat those that cannot help them.``"};
								if (tile == 5750) lunar_new_year.push_back(9286);
								int rand_item = lunar_new_year[rand() % lunar_new_year.size()];
								string rand_message = lunar_messages[rand() % lunar_messages.size()];
								int count = 1;
								if (rand_item == 5768) count = 4;
								if (rand_item == 5766) count = 3;
								if (rand_item == 5744 || rand_item == 9290) count = 8;
								if (rand_item == 7696 || rand_item == 9272 || rand_item == 5754 || rand_item == 10576) {
									int target = 5;
									if (tile == 9286) target = 10;
									if ((rand() % 1000) <= target) { }
									else rand_item = 5744;
								}
								Player::OnTalkBubble(peer, pData->netID, "You received `2" + to_string(count) + " " + getItemDef(rand_item).name + "`` from the Lucky Fortune Cookie.", 0, false);
								Player::OnConsoleMessage(peer, "You received `2" + to_string(count) + " " + getItemDef(rand_item).name + "`` from the Lucky Fortune Cookie.");
								Player::OnTalkBubble(peer, pData->netID, rand_message, 0, false);
								Player::OnConsoleMessage(peer, rand_message);
								bool success = true;
								SaveItemMoreTimes(rand_item, count, peer, success);
								break;
							} else if (x == static_cast<PlayerInfo*>(currentPeer->data)->x / 32 && y == static_cast<PlayerInfo*>(currentPeer->data)->y / 32) {
								Player::OnTalkBubble(peer, pData->netID, "You can only use that on yourself.", 0, true);
								break;
							} else {
								Player::OnTalkBubble(peer, pData->netID, "Must be used on a person.", 0, true); 
							}
						}
					}
				}
				if(tile == 4598 || tile == 4596 || tile == 7056 || tile == 6316 || tile == 4594 || tile == 4604)
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						if(pData->food == 100)
						{
							Player::OnTalkBubble(peer, pData->netID, "`wSuanda ac degilim!", 0, true);
							return;
						}
						int hp = -1;
						ifstream infile("homeoven.txt");
						for (string line; getline(infile, line);) {
							if (line.length() > 3 && line.at(0) != '/' && line.at(1) != '/') {
								auto ex = explode("|", line);
								if (atoi(ex.at(8).c_str()) == tile)
								{
									hp = atoi(ex.at(9).c_str());
									break;
								}
							}
						}
						if (hp == -1)
						{
							Player::OnTalkBubble(peer, pData->netID, "`4HATA OLUSTU!", 0, true);
							return;
						}
						Player::OnTalkBubble(peer, pData->netID, "`oAclik seviyen `1" + to_string(pData->food) + "`o den `1" + to_string(min(pData->food + hp, 100)) + " yukseldi.", 0, true);
						pData->food = min(pData->food + hp, 100);
						RemoveInventoryItem(tile, 1, peer, true);
					}
				}
				if (tile == 6918) /*punchpotion*/
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						if (!pData->PunchPotion)
						{
							SendTradeEffect(peer, 6918, pData->netID, pData->netID, 150);
							sendSound(peer, "audio/spray.wav");
							RemoveInventoryItem(6918, 1, peer, true);
							Player::OnConsoleMessage(peer, "You're `$stronger `othan before! (`$One HIT! `omod added, `$10 mins`o left)");
							pData->usedPunchPotion = (GetCurrentTimeInternalSeconds() + (10 * 60));
							pData->PunchPotion = true;
						}
						else Player::OnTalkBubble(peer, pData->netID, "You already have active punch potion!", 0, true);
					}
				}
				if (tile == 6924) /*placepotion*/
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (x == pData->x / 32 && y == pData->y / 32)
					{
						if (pData->PlacePotion == false)
						{
							SendTradeEffect(peer, 6924, pData->netID, pData->netID, 150);
							sendSound(peer, "audio/spray.wav");
							RemoveInventoryItem(6924, 1, peer, true);
							Player::OnConsoleMessage(peer, "Your hands are `$exceeding`o! (`$Triple Place! `omod added, `$10 mins`o left)");
							pData->usedPlacePotion = (GetCurrentTimeInternalSeconds() + (10 * 60));
							pData->PlacePotion = true;
						}
						else Player::OnTalkBubble(peer, pData->netID, "You already have active place potion!", 0, true);
					}
				}
				if (tile == 540)
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					RemoveInventoryItem(540, 1, peer, true);
					Player::OnTalkBubble(peer, pData->netID, "`2BURRRPPP...!", 0, true);
				}
				if (tile == 3212)
				{
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (world->items.at(x + (y * world->width)).fire == false && world->items.at(x + (y * world->width)).water == false)
					{
						if (isSeed(world->items.at(x + (y * world->width)).foreground)  || world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 6864 || world->items.at(x + (y * world->width)).water || world->items.at(x + (y * world->width)).foreground == 6952 || world->items.at(x + (y * world->width)).foreground == 6954 || world->items.at(x + (y * world->width)).foreground == 5638 || world->items.at(x + (y * world->width)).foreground == 6946 || world->items.at(x + (y * world->width)).foreground == 6948 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::VENDING || world->items.at(x + (y * world->width)).foreground == 1420 || world->items.at(x + (y * world->width)).foreground == 6214 || world->items.at(x + (y * world->width)).foreground == 1006 || world->items.at(x + (y * world->width)).foreground == 656 || world->items.at(x + (y * world->width)).foreground == 1420 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DONATION || world->items.at(x + (y * world->width)).foreground == 3528 || world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0 || world->items.at(x + (y * world->width)).foreground == 6 || world->items.at(x + (y * world->width)).foreground == 8 || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DISPLAY) {
							if (world->items.at(x + (y * world->width)).background != 6864) Player::OnTalkBubble(peer, pData->netID, "`wBunu yakamazsin!", 0, true);
							return;
						}
						if (world->items.at(x + (y * world->width)).foreground != 6 && world->items.at(x + (y * world->width)).foreground != 8 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::LOCK)
						{
							if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0)
							{
								Player::OnTalkBubble(peer, pData->netID, "`wYakacak hic biryer yok!", 0, true);
								return;
							}
							world->items.at(x + (y * world->width)).fire = !world->items.at(x + (y * world->width)).fire;
							ENetPeer* net_peer;
							for (net_peer = server->peers;
								net_peer < &server->peers[server->peerCount];
								++net_peer)
							{
								if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, net_peer))
								{
									Player::OnParticleEffect(net_peer, 150, x * 32 + 16, y * 32 + 16, 0);
									Player::OnTalkBubble(net_peer, pData->netID, "`7[```4QWEQWEQWEEWQ!! ATES ATES ATES```7]``", 0, false);
								}
							}
							RemoveInventoryItem(3062, 1, peer, true);
							UpdateVisualsForBlock(peer, true, x, y, world);
							if (world->items.at(x + (y * world->width)).foreground == 0)
							{
								if (getItemDef(tile).rarity != 999)
								{
									int b = getGemCount(tile) + rand() % 1;
									while (b > 0)
									{
										if (b >= 100)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												100, 0);
											b -= 100;
											for (int i = 0; i < rand() % 1; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 10, 0);
											for (int i = 0; i < rand() % 4; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
										if (b >= 50)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												50, 0);
											b -= 50;
											for (int i = 0; i < rand() % 1; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 5, 0);
											for (int i = 0; i < rand() % 3; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
										if (b >= 10)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												10, 0);
											b -= 10;
											for (int i = 0; i < rand() % 4; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
										if (b >= 7)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												10, 0);
											b -= 5;
											for (int i = 0; i < rand() % 2; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
										if (b >= 5)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												5, 0);
											b -= 5;
											for (int i = 0; i < rand() % 2; i++) DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
										if (b >= 1)
										{
											DropItem(
												world, peer, -1,
												x * 32 + (rand() % 16),
												y * 32 + (rand() % 16),
												112,
												1, 0);
											b -= 1;
											for (int i = 0; i < rand() % 1; i++) DropItem(world, peer, -1, x * 32 + (rand() % 8), y * 32 + (rand() % 16), 112, 1, 0);
											continue;
										}
									}
								}
							}
						}
					} 
					return;
				}
				if (tile == 822) {
					if (world->isPineappleGuard == true) {
						Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
						return;
					}
					if (world->items.at(x + (y * world->width)).fire) {
						SendThrowEffect(peer, 822, pData->netID, -1, 150, 0, x * 32 + 16, y * 32 + 16);
						RemoveInventoryItem(822, 1, peer, true);
						world->items.at(x + (y * world->width)).fire = false;
						UpdateBlockState(peer, x, y, true, world);
						for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
							if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
							if (isHere(peer, net_peer)) {
								Player::OnParticleEffect(net_peer, 149, x * 32, y * 32, 0);
							}
						}
						return;
					}
					if (world->items.at(x + (y * world->width)).foreground != 6 && world->items.at(x + (y * world->width)).foreground != 8 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::LOCK) {
						if (world->isPineappleGuard == true) {
							Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
							return;
						}
						if (isWorldOwner(peer, world) || world->owner == "" || isDev(peer)) {
							if (world->items.at(x + (y * world->width)).water) {
								world->items.at(x + (y * world->width)).water = false;
								UpdateBlockState(peer, x, y, false, world);
								if ((rand() % 99) + 1 < 40) {
									bool success = true;
									SaveItemMoreTimes(822, 1, peer, success);
								}
								UpdateVisualsForBlock(peer, false, x, y, world);
							} else { 
								world->items.at(x + (y * world->width)).water = true;
								UpdateBlockState(peer, x, y, true, world);
								RemoveInventoryItem(822, 1, peer, true);
								SendThrowEffect(peer, 822, pData->netID, -1, 150, 0, x * 32 + 16, y * 32 + 16);
								UpdateVisualsForBlock(peer, true, x, y, world);
							}
						}
					}
				}
				return;
			}
		case 1866:
			{
				if (world->isPineappleGuard == true) {
					Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
					return;
				}
				if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer)) {
					if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
						return;
					}
					world->items.at(x + (y * world->width)).glue = !world->items.at(x + (y * world->width)).glue;
					UpdateVisualsForBlock(peer, true, x, y, world);
				}
				return;
			}
		case 834:
		{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
			if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer)) {
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
					return;
				}
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
					if (isHere(peer, currentPeer)) {
						Player::PlayAudio(currentPeer, "audio/launch.wav", 0);
						Player::OnParticleEffect(currentPeer, 48, static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y - 5, 0);
						Player::OnParticleEffect(currentPeer, 37, static_cast<PlayerInfo*>(peer->data)->x - 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
						Player::OnParticleEffect(currentPeer, 38, static_cast<PlayerInfo*>(peer->data)->x + 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
						Player::OnParticleEffect(currentPeer, 39, static_cast<PlayerInfo*>(peer->data)->x - 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
						Player::OnParticleEffect(currentPeer, 40, static_cast<PlayerInfo*>(peer->data)->x + 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
						Player::OnParticleEffect(currentPeer, 37, static_cast<PlayerInfo*>(peer->data)->x - 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
						Player::OnParticleEffect(currentPeer, 38, static_cast<PlayerInfo*>(peer->data)->x + 20 * (rand() % 12), static_cast<PlayerInfo*>(peer->data)->y - 30 * (rand() % 12 + 1), 2500);
					}
				}
				RemoveInventoryItem(834, 1, peer, true);
			}
			return;
		}
		case 1680:
		{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
			if (world->owner == "" || isWorldOwner(peer, world) || isDev(peer)) {
				if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
					return;
				}
				bool iscontainseas = false;
				SearchInventoryItem(peer, 834, 250, iscontainseas);
				if (!iscontainseas)
				{
					Player::OnTalkBubble(peer, pData->netID, "`wYou don't have `0250 `0Fireworks for launch `4Super Firework!", false, 0);
					return;
				}

				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
					if (isHere(peer, currentPeer)) {
						Player::PlayAudio(currentPeer, "audio/launch.wav", 0);
						Player::OnParticleEffect(currentPeer, 73, static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y - 25 * (rand() % 12 + 1), 3);
					}
				}
				RemoveInventoryItem(1680, 1, peer, true);
				RemoveInventoryItem(834, 250, peer, true);
				int items[64] = { 3764,1664,6308,6306,4814,9732,6322,4818,2874,8590,4820,8588,2854,9730,6312,1674,2802,3696,1678,1666,2870,2872,8618,8616,4816,1676,2864,1670,4822,844,1668,6310,2868,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004,1004 };
				DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), items[rand() % 34], 1, 0);
			}
			return;
			}
		case 3562:
			{
				if (world->isPineappleGuard == true) {
					Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
					return;
				}
				Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wCave Blast`|left|3562|\nadd_textbox|This item creates a new world!  Enter a unique name for it.|left|\nadd_text_input|cavename|New World Name||24|\nend_dialog|usecaveblast|Cancel|`5Create!|\n");
				return;
			}
		case 830:
		{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
			bool iscontainseas = false;
			SearchInventoryItem(peer, 834, 100, iscontainseas);
			if (!iscontainseas)
			{
				Player::OnTalkBubble(peer, pData->netID, "`wYou don't have `1100 Fireworks", false, 0);
				return;
			}
			Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wBeach Blast`|left|830|\nadd_textbox|This item creates a new world! Enter a unique name for it.|left|\nadd_text_input|beachname|New World Name||24|\nend_dialog|usebeachblast|Cancel|`5Create!|\n");
			return;
		}
		case 1136:
		{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
			Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wMars Blast`|left|1136|\nadd_textbox|This item creates a new world! Enter a unique name for it.|left|\nadd_text_input|marsname|New World Name||24|\nend_dialog|usemarsblast|Cancel|`5Create!|\n");
			return;
		}
		case 1532:
		{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
			Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wUndersea Blast`|left|1532|\nadd_textbox|This item creates a new world! Enter a unique name for it.|left|\nadd_text_input|seaname|New World Name||24|\nend_dialog|useseablast|Cancel|`5Create!|\n");
			return;
		}
		case 77859484:
			{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
				Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wTiny Blast`|left|7784|\nadd_textbox|This item creates a new world!  Enter a unique name for it.|left|\nadd_text_input|tinyname|New World Name||24|\nend_dialog|usetinyblast|Cancel|`5Create!|\n");
				return;
			}
		case 26473:
			{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
				Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wLarge Blast`|left|7562|\nadd_textbox|This item creates a new world!  Enter a unique name for it.|left|\nadd_text_input|largename|New World Name||24|\nend_dialog|uselargeblast|Cancel|`5Create!|\n");
				return;
			}
		case 1402:
			{
				if (world->isPineappleGuard == true) {
					Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
					return;
				}
				Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wThermonuclear Blast`|left|1402|\nadd_textbox|This item creates a new world!  Enter a unique name for it.|left|\nadd_text_input|thermoname|New World Name||24|\nend_dialog|usethermoblast|Cancel|`5Create!|\n");
				return;
			}
		case 7588:
			{
			if (world->isPineappleGuard == true) {
				Player::OnTalkBubble(peer, pData->netID, "`wYou cant use consumables here!", false, 0);
				return;
			}
				Player::OnDialogRequest(peer, "set_default_color|`o\n\nadd_label_with_icon|big|`wTreasure Blast`|left|7588|\nadd_textbox|This item creates a new world!  Enter a unique name for it.|left|\nadd_text_input|treasurename|New World Name||24|\nend_dialog|usetreasureblast|Cancel|`5Create!|\n");
				return;
			}
		case 1826:
			{
				if (!isWorldOwner(peer, world)) return;
				auto iscontainsss = false;
				SearchInventoryItem(peer, 1826, 1, iscontainsss);
				if (!iscontainsss) {
					return;
				} else {
					auto FoundSomething = false;
					for (auto i = 0; i < world->width * world->height; i++) {
						if (isSeed(world->items.at(i).foreground)) {
							sendTileUpdate((i % world->width), (i / world->width), 18, pData->netID, peer, world);
							ENetPeer* net_peer;
							for (net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
								if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, net_peer)) {
									Player::OnParticleEffect(net_peer, 182, (i % world->width) * 32, (static_cast<float>(i) / world->width) * 32, 0);
								}
							}
							FoundSomething = true;
						}
					}
					if (FoundSomething) RemoveInventoryItem(1826, 1, peer, true);
				}
				return;
			}
		case 5640: /*magplantremote*/
			{
				bool aryra = false;
				for (int i = 0; i < world->width * world->height; i++) {
					if (world->items.at(i).foreground == 5638) {
						aryra = true;
					}
				}
				if (aryra == true) {
					if (pData->magplantx != 0 && pData->magplanty != 0) {
						int squaresign = pData->magplantx + (pData->magplanty * world->width);
						string currentworld = pData->currentWorld + "X" + std::to_string(squaresign);
						if (world->items.at(pData->magplantx + (pData->magplanty * world->width)).mid == pData->magplantitemid && world->items.at(pData->magplantx + (pData->magplanty * world->width)).mc > 0) {
							int magplantid = pData->magplantitemid;
							bool RotatedRight = false;
							auto xpos = x * 32;
							auto ppos = pData->x;
							if (pData->x < x * 32) RotatedRight = true;
							if (RotatedRight) ppos += 19;
							xpos = xpos / 32;
							ppos = ppos / 32;
							if (world->items.at(x + (y * world->width)).foreground != 0 && getItemDef(magplantid).blockType != BlockTypes::BACKGROUND && getItemDef(magplantid).blockType != BlockTypes::GROUND_BLOCK) return;
							ENetPeer* currentPeer;
							for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer)) {
									bool RotatedRight = false;
									auto xpos = x * 32;
									auto ppos = static_cast<PlayerInfo*>(currentPeer->data)->x;
									if (static_cast<PlayerInfo*>(currentPeer->data)->x < x * 32) RotatedRight = true;
									if (RotatedRight) ppos += 19;
									xpos = xpos / 32;
									ppos = ppos / 32;
									if (ppos == xpos && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == y && getItemDef(magplantid).properties != Property_NoSeed && getItemDef(magplantid).properties != Property_Foreground && getItemDef(magplantid).properties != Property_MultiFacing && getItemDef(magplantid).blockType != BlockTypes::SEED && getItemDef(magplantid).blockType != BlockTypes::STEAM && getItemDef(magplantid).blockType != BlockTypes::UNKNOWN && getItemDef(magplantid).blockType != BlockTypes::VENDING && getItemDef(magplantid).blockType != BlockTypes::ANIM_FOREGROUND && getItemDef(magplantid).blockType != BlockTypes::BULLETIN_BOARD && getItemDef(magplantid).blockType != BlockTypes::FACTION && getItemDef(magplantid).blockType != BlockTypes::CHEST && getItemDef(magplantid).blockType != BlockTypes::GEMS && getItemDef(magplantid).blockType != BlockTypes::MAGIC_EGG && getItemDef(magplantid).blockType != BlockTypes::growplay && getItemDef(magplantid).blockType != BlockTypes::MAILBOX && getItemDef(magplantid).blockType != BlockTypes::PORTAL && getItemDef(magplantid).blockType != BlockTypes::PLATFORM && getItemDef(magplantid).blockType != BlockTypes::SFX_FOREGROUND && getItemDef(magplantid).blockType != BlockTypes::CHEMICAL_COMBINER && getItemDef(magplantid).blockType != BlockTypes::SWITCH_BLOCK && getItemDef(magplantid).blockType != BlockTypes::TRAMPOLINE && getItemDef(magplantid).blockType != BlockTypes::TOGGLE_FOREGROUND && getItemDef(magplantid).blockType != BlockTypes::GROUND_BLOCK && getItemDef(magplantid).blockType != BlockTypes::BACKGROUND && getItemDef(magplantid).blockType != BlockTypes::MAIN_DOOR && getItemDef(magplantid).blockType != BlockTypes::SIGN && getItemDef(magplantid).blockType != BlockTypes::DOOR && getItemDef(magplantid).blockType != BlockTypes::CHECKPOINT && getItemDef(magplantid).blockType != BlockTypes::GATEWAY && getItemDef(magplantid).blockType != BlockTypes::TREASURE && getItemDef(magplantid).blockType != BlockTypes::WEATHER) return;
								}
							}
							if (world->isPublic || isWorldAdmin(peer, world) || pData->rawName == world->owner || world->owner == "" || isDev(peer) || !restricted_area(peer, world, x, y)) {
								world->items.at(x + (y * world->width)).foreground = magplantid;
								world->items.at(pData->magplantx + (pData->magplanty * world->width)).mc -= 1;
								PlayerMoving data3{};
								data3.packetType = 0x3;
								data3.characterState = 0x0;
								data3.x = x;
								data3.y = y;
								data3.punchX = x;
								data3.punchY = y;
								data3.XSpeed = 0;
								data3.YSpeed = 0;
								data3.netID = -1;
								data3.plantingTree = magplantid;
								for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
									if (isHere(peer, currentPeer)) {
										auto raw = packPlayerMoving(&data3);
										raw[2] = dicenr;
										raw[3] = dicenr;
										SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
									}
								}
								for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
									if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
									if (isHere(peer, currentPeer)) {
										if (world->items.at(pData->magplantx + (pData->magplanty * world->width)).mc <= 0) {
											send_item_sucker(currentPeer, 5638, pData->magplantx, pData->magplanty, world->items.at(pData->magplantx + (pData->magplanty * world->width)).mid, 0, true, true, world->items.at(pData->magplantx + (pData->magplanty * world->width)).background);
										}
										else if (world->items.at(pData->magplantx + (pData->magplanty * world->width)).mc >= 5000) {
											send_item_sucker(currentPeer, 5638, pData->magplantx, pData->magplanty, world->items.at(pData->magplantx + (pData->magplanty * world->width)).mid, -1, true, true, world->items.at(pData->magplantx + (pData->magplanty * world->width)).background);
										}
										else {
											send_item_sucker(currentPeer, 5638, pData->magplantx, pData->magplanty, world->items.at(pData->magplantx + (pData->magplanty * world->width)).mid, 1, true, true, world->items.at(pData->magplantx + (pData->magplanty * world->width)).background);
										}
									}
								}
							}
						}
						else {
							Player::OnTalkBubble(peer, pData->netID, "`wThe `2MAGPLANT 5000 `wis empty!", 0, false);
						}
					}
				}
				return;
			}
		case 6204: case 6202: case 6250: case 7484: case 7954: case 1360: case 11398: case 5402: /*chest*/
		{
			if (tile == 11398)
			{
				SearchInventoryItem(peer, 11398, 1, iscontains);
				if (!iscontains) return;
				else
				{
					RemoveInventoryItem(11398, 1, peer, true);
					int itemuMas[107] = { 10956, 10954, 10958, 10952, 10960, 10952, 10954, 10990, 10958, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448, 10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448,  10990, 10996, 10994, 11422, 11410, 11426, 11408, 10998, 11000, 11452, 11448, };
					auto randIndex = rand() % 107;
					int itemid = itemuMas[randIndex];
					bool success = true;
					SaveItemMoreTimes(itemid, 1, peer, success, pData->rawName + " from " + getItemDef(tile).name + "");
					Player::OnConsoleMessage(peer, "`oYou received `2" + getItemDef(itemid).name + " `ofrom the Alien Landing Pod!");
					Player::OnTalkBubble(peer, pData->netID, "`oYou received `2" + getItemDef(itemid).name + " `ofrom the Alien Landing Pod!", 0, true);
					if (itemid == 10958 || itemid == 10954 || itemid == 10956 || itemid == 10960 || itemid == 10952) {
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							Player::OnConsoleMessage(currentPeer, "`oAn `4Alien Landing Pod `orewards `2" + pData->rawName + " `owith a rare `5" + getItemDef(itemid).name);
						}
					}
				}
			}

			if (tile == 1360) {
				ENetPeer* currentPeerxd;
				for (currentPeerxd = server->peers;
					currentPeerxd < &server->peers[server->peerCount];
					++currentPeerxd)
				{
					if (currentPeerxd->state != ENET_PEER_STATE_CONNECTED)
						continue;
					if (isHere(peer, currentPeerxd)) {
						string xtest = to_string(((PlayerInfo*)(currentPeerxd->data))->x / 32);
						string ytest = to_string(((PlayerInfo*)(currentPeerxd->data))->y / 32);
						int xt1 = atoi(xtest.c_str());
						int xt2 = atoi(ytest.c_str());
						if (x == xt1 && y == xt2) {
							std::vector<int> list{ 2, 14, 10, 2, 14, 10, 22, 24, 26, 28, 60, 62, 64, 100, 102, 106, 108, 110, 114, 116, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194 };
							int c = rand() % list.size();
							int x = list[c];
							if (((PlayerInfo*)(peer->data))->inventory.items.size() == ((PlayerInfo*)(peer->data))->currentInventorySize) {
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`oYou need more inventory space!", 0, true);
								return;
							}
							short int currentItemCount = 0;
							for (int i = 0; i < ((PlayerInfo*)(peer->data))->inventory.items.size(); i++) {
								if (((PlayerInfo*)(peer->data))->inventory.items.at(i).itemID == x) {
									currentItemCount = (unsigned int)((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount;
									if (currentItemCount < 0) {
										currentItemCount = FixCountItem((unsigned int)((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount);
									}
								}
							}
							int plusItem = currentItemCount + 1;
							if (plusItem > 250) {
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`wOops - You is carrying too many " + getItemDef(x).name + " and can't fit that many in their backpack.", 0, true);
								return;
							}
							else {
								bool success = true;
								SaveItemMoreTimes(x, 1, peer, success);
								RemoveInventoryItem(1360, 1, peer, true);
								Player::OnConsoleMessage(peer, "`$There's 1 `2" + getItemDef(x).name + " `$in the package!");
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`$There's 1 `2" + getItemDef(x).name + " `$in the package!", 0, true);
								SendTradeEffect(peer, tile, ((PlayerInfo*)(peer->data))->netID, ((PlayerInfo*)(currentPeerxd->data))->netID, 180);
								SendTradeEffect(currentPeerxd, tile, ((PlayerInfo*)(peer->data))->netID, ((PlayerInfo*)(currentPeerxd->data))->netID, 180);
							}
						}
						else {
							Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "Must be used on a person.", 0, true);
						}
					}
				}
			}
			if (tile == 5402) {
				ENetPeer* currentPeerxd;
				for (currentPeerxd = server->peers;
					currentPeerxd < &server->peers[server->peerCount];
					++currentPeerxd)
				{
					if (currentPeerxd->state != ENET_PEER_STATE_CONNECTED)
						continue;
					if (isHere(peer, currentPeerxd)) {
						string xtest = to_string(((PlayerInfo*)(currentPeerxd->data))->x / 32);
						string ytest = to_string(((PlayerInfo*)(currentPeerxd->data))->y / 32);
						int xt1 = atoi(xtest.c_str());
						int xt2 = atoi(ytest.c_str());
						if (x == xt1 && y == xt2) {
							std::vector<int> list{ 7448, 7446, 5372, 3204, 7438, 5470, 5396, 5368, 10404, 5370, 11516, 5478, 5400, 7436, 5358, 5360, 5362, 5386, 5364, 9196, 7454, 5472, 228, 11484, 5476, 5352, 5350, 5354, 5348, 1778, 7458, 5384, 5474, 9180, 5394, 7448, 7446, 5372, 3204, 7438, 5470, 5396, 5368, 10404, 5370, 11516, 5478, 5400, 7436, 5358, 5360, 5362, 5386, 5364, 9196, 7454, 5472, 228, 11484, 5476, 5352, 5350, 5354, 5348, 1778, 7458, 5384, 5474, 9180, 5394, 7448, 7446, 5372, 3204, 7438, 5470, 5396, 5368, 10404, 5370, 11516, 5478, 5400, 7436, 5358, 5360, 5362, 5386, 5364, 9196, 7454, 5472, 228, 11484, 5476, 5352, 5350, 5354, 5348, 1778, 7458, 5384, 5474, 9180, 5394, 7448, 7446, 5372, 3204, 7438, 5470, 5396, 5368, 10404, 5370, 11516, 5478, 5400, 7436, 5358, 5360, 5362, 5386, 5364, 9196, 7454, 5472, 228, 11484, 5476, 5352, 5350, 5354, 5348, 1778, 7458, 5384, 5474, 9180, 5394, 5404, 5404 };
							int c = rand() % list.size();
							int x = list[c];
							if (((PlayerInfo*)(peer->data))->inventory.items.size() == ((PlayerInfo*)(peer->data))->currentInventorySize) {
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`oDaha fazla envanter slotuna ihtiyaciniz var!", 0, true);
								return;
							}
							short int currentItemCount = 0;
							for (int i = 0; i < ((PlayerInfo*)(peer->data))->inventory.items.size(); i++) {
								if (((PlayerInfo*)(peer->data))->inventory.items.at(i).itemID == x) {
									currentItemCount = (unsigned int)((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount;
									if (currentItemCount < 0) {
										currentItemCount = FixCountItem((unsigned int)((PlayerInfo*)(peer->data))->inventory.items.at(i).itemCount);
									}
								}
							}
							int plusItem = currentItemCount + 1;
							if (plusItem > 250) {
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`wEyvah - Suanda envanterinde cok fazla " + getItemDef(x).name + " tasiyorsun ve envanterinize sigmiyor lutfen yer acin.", 0, true);
								return;
							}
							else {
								bool success = true;
								SaveItemMoreTimes(x, 1, peer, success);
								RemoveInventoryItem(5402, 1, peer, true);
								Player::OnConsoleMessage(peer, "`oMutlu Kisfestivali, `o" + getItemDef(x).name + " `okazandin");
								Player::PlayAudio(peer, "audio/cracker_bang.wav", 0);
								SendParticleEffect(peer, x * 32 + 16, y * 32 + 16, 1953289573, 168, 0);
								SendTradeEffect(peer, tile, ((PlayerInfo*)(peer->data))->netID, ((PlayerInfo*)(currentPeerxd->data))->netID, 5402);
								SendTradeEffect(currentPeerxd, tile, ((PlayerInfo*)(peer->data))->netID, ((PlayerInfo*)(currentPeerxd->data))->netID, 5402);
							}
						}
						else {
							//Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "`2Kisfestivali ruhunuzu canlandirin ve sevgi'yi paylasin...", 0, true);
						}
					}
				}
			}
				if (tile == 6204) {
					if (pData->haveGrowId == false) return;
					auto iscontains = false;
					SearchInventoryItem(peer, 6204, 1, iscontains);
					if (!iscontains) {
						return;
					} else {
						RemoveInventoryItem(6204, 1, peer, true);
						auto kuriPrizaDuot = rand() % 2 + 1;
						if (kuriPrizaDuot == 1)
						{
							auto gemChance = rand() % 25000;
							GiveChestPrizeGems(peer, gemChance);
						}
						if (kuriPrizaDuot == 2)
						{
							int itemuMas[5] = { 7912, 7912, 7912, 5078, 8834 };
							auto randIndex = rand() % 5;
							auto itemId = itemuMas[randIndex];
							send_item(peer, itemId, 1, 6204);
						}
					}
				}
				if (tile == 6202)
				{
					if (pData->haveGrowId == false) return;
					auto iscontains = false;
					SearchInventoryItem(peer, 6202, 1, iscontains);
					if (!iscontains)
					{
						return;
					}
					else
					{
						RemoveInventoryItem(6202, 1, peer, true);
						auto kuriPrizaDuot = rand() % 2 + 1;
						if (kuriPrizaDuot == 1)
						{
							auto gemChance = rand() % 70000;
							GiveChestPrizeGems(peer, gemChance);
						}
						if (kuriPrizaDuot == 2)
						{
							int itemuMas[5] = { 7912, 5078, 5078, 5078, 8834 };
							auto randIndex = rand() % 5;
							auto itemId = itemuMas[randIndex];
							send_item(peer, itemId, 1, 6202);
						}
					}
				}
				if (tile == 6250)
				{
					if (pData->haveGrowId == false) return;
					auto iscontains = false;
					SearchInventoryItem(peer, 6250, 1, iscontains);
					if (!iscontains)
					{
						return;
					}
					else
					{
						RemoveInventoryItem(6250, 1, peer, true);
						auto kuriPrizaDuot = rand() % 2 + 1;
						if (kuriPrizaDuot == 1)
						{
							auto gemChance = rand() % 125000;
							GiveChestPrizeGems(peer, gemChance);
						}
						if (kuriPrizaDuot == 2)
						{
							int itemuMas[8] = { 7912, 7912, 7912, 7912, 5078, 5078, 5078, 8834 };
							auto randIndex = rand() % 8;
							auto itemId = itemuMas[randIndex];
							send_item(peer, itemId, 1, 6250);
						}
					}
				}
				if (tile == 7484)
				{
					if (pData->haveGrowId == false) return;
					auto iscontains = false;
					SearchInventoryItem(peer, 7484, 1, iscontains);
					if (!iscontains)
					{
						return;
					}
					else
					{
						RemoveInventoryItem(7484, 1, peer, true);
						auto kuriPrizaDuot = rand() % 2 + 1;
						if (kuriPrizaDuot == 1)
						{
							auto gemChance = rand() % 170000;
							GiveChestPrizeGems(peer, gemChance);
						}
						if (kuriPrizaDuot == 2)
						{
							int itemuMas[12] = { 7912, 7912, 7912, 7912, 5078, 5078, 5078, 8834, 8834, 8834, 8834, 8834 };
							auto randIndex = rand() % 12;
							int itemId = itemuMas[randIndex];
							send_item(peer, itemId, 1, 7484);
						}
					}
				}
				if (tile == 7954)
				{
					if (pData->haveGrowId == false) return;
					auto iscontains = false;
					SearchInventoryItem(peer, 7954, 1, iscontains);
					if (!iscontains)
					{
						return;
					}
					else
					{
						RemoveInventoryItem(7954, 1, peer, true);
						auto kuriPrizaDuot = rand() % 2 + 1;
						if (kuriPrizaDuot == 1)
						{
							int gemChance = rand() % 250000;
							GiveChestPrizeGems(peer, gemChance);
						}
						if (kuriPrizaDuot == 2)
						{
							int itemuMas[12] = { 7912, 7912, 7912, 7912, 5078, 5078, 5078, 8834, 8834, 8834, 8834, 8834 };
							auto randIndex = rand() % 12;
							auto itemId = itemuMas[randIndex];
							send_item(peer, itemId, 1, 7954);
						}
					}
				}
				return;
			}
		case 1404: /*doormover*/			
			{			
			if (pData->rawName != world->owner && !isDev(peer) && world->owner != "") return;			
				if (world->items.at(x + (y * world->width)).foreground != 0 || world->items.at(x + (y * world->width + world->width)).foreground != 0) {
					Player::OnTextOverlay(peer, "Beyaz kapi icin burada bir bosluk yok!");
					return;
				} else {
					RemoveInventoryItem(1404, 1, peer, true);						
					Player::OnTalkBubble(peer, pData->netID, "Kapinin yerini degistirdin!", 0, true);			
					for (int i = 0; i < world->width * world->height; i++) {
						if (world->items.at(i).foreground == 6) {
							world->items.at(i).foreground = 0;
							thread_safe_tileupdate.push_back(world->name + "|" + to_string(i % world->width) + "|" + to_string(i / world->width) + "|" + to_string(0));									
							if (world->items.at((i % world->width) + ((i / world->width) * world->width + world->width)).foreground == 8) {									
								world->items.at((i % world->width) + ((i / world->width) * world->width + world->width)).foreground = 0;
								thread_safe_tileupdate.push_back(world->name + "|" + to_string(i % world->width) + "|" + to_string((i / world->width) + 1) + "|" + to_string(0));									
							} for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, currentPeer)) {
									static_cast<PlayerInfo*>(currentPeer->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 2;
									static_cast<PlayerInfo*>(currentPeer->data)->disableanticheat = true;
									static_cast<PlayerInfo*>(currentPeer->data)->lastx = 0;
									static_cast<PlayerInfo*>(currentPeer->data)->lasty = 0;
									Player::OnSetPos(currentPeer, static_cast<PlayerInfo*>(currentPeer->data)->netID, x * 32, y * 32);
								}
							}
							break;
						} 
					}
					world->items.at(x + (y * world->width)).foreground = 6;	
					world->items.at(x + (y * world->width + world->width)).foreground = 8;	
					thread_safe_tileupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(6));																	
					thread_safe_tileupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y + 1) + "|" + to_string(8));																	
				}
				return;
			}
		case 5460: case 4520: case 382: case 3116: case 2994: case 4368: case 5708: case 5709: case 5780: case 5781: case 5782: case 5783: case 5784: case 5785: case 5710: case 5711: case 5786: case 5787: case 5788: case 5789: case 5790: case 5791: case 6146: case 6147: case 6148: case 6149: case 6150: case 6151: case 6152: case 6153: case 5670: case 5671: case 5798: case 5799: case 5800: case 5801: case 5802: case 5803: case 5668: case 5669: case 5792: case 5793: case 5794: case 5795: case 5796: case 5797: case 544: case 54600: case 1902: case 1508: case 3808: case 5132: case 7166: case 5078: case 5080: case 5082: case 5084: case 5126: case 5128: case 5130: case 5144: case 5146: case 5148: case 5150: case 5162: case 5164: case 5166: case 5168: case 5180: case 5182: case 5184: case 5186: case 7168: case 7170: case 7172: case 7174: case 2480: case 9999: case 980: case 3212: case 4742: case 3496: case 3270: case 9212: case 5134: case 5152: case 5170: case 5188: case 611:
			{
				return;
			}
		default:
			{
				if (world->items.at(x + (y * world->width)).fire && tile != 18) return;
				if (tile == 6954) {
					isMagplant = true;
				}
				if (getItemDef(tile).blockType == BlockTypes::PROVIDER) {
					isScience = true;
				}
				if (getItemDef(tile).blockType == BlockTypes::DONATION)
				{
					namespace fs = std::experimental::filesystem;
					if (!fs::is_directory("save/donationboxes/_" + world->name) || !fs::exists("save/donationboxes/_" + world->name))
					{
						fs::create_directory("save/donationboxes/_" + world->name);
					}
					ofstream of("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
					json j;
					j["donated"] = 0;
					json jArray = json::array();
					json jmid;
					for (int i = 1; i <= 20; i++)
					{
						jmid["aposition"] = i;
						jmid["sentBy"] = "";
						jmid["note"] = "";
						jmid["itemid"] = 0;
						jmid["itemcount"] = 0;
						jArray.push_back(jmid);
					}
					j["donatedItems"] = jArray;
					of << j << std::endl;
					of.close();
				}
				//entrance place
				if (getItemDef(tile).blockType == BlockTypes::GATEWAY) {
					world->items.at(squaresign).opened = false;
					isgateway = true;
				}
				if(getItemDef(tile).blockType == BlockTypes::DOOR || getItemDef(tile).blockType == BlockTypes::PORTAL)
				{
					world->items.at(squaresign).opened = false;
				}
				if (tile == 3798) {
					isvip = true;
					world->items[squaresign].opened = false;
				}
				if (getItemDef(tile).blockType == BlockTypes::DOOR) {
					world->items.at(x + (y * world->width)).label = getItemDef(tile).name;
					isDoor = true;
				}
				if (world->items.at(x + (y * world->width)).foreground == 1436) /*securitycamera*/
				{
					bool aryra = false;
					for (int i = 0; i < world->width * world->height; i++)
					{
						if (world->items[i].foreground == 1436)
						{
							aryra = true;
						}
					}
					if (aryra == false)
					{
						if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner)
						{
							Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`0You need to be world owner to place `cSecurity Camera`0!", 0, true);
							return;
						}
					}
					else
					{
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`0You cant place more than one `cSecurity Camera`0!", 0, true);
						return;
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 1420 || world->items.at(x + (y * world->width)).foreground == 6214 && tile != 18) {
					if (getItemDef(tile).blockType == BlockTypes::CLOTHING)
					{
						if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner))
						{
							if (getItemDef(tile).properties & Property_Untradable) {
								Player::OnTalkBubble(peer, pData->netID, "You can't use untradeable items with mannequins.", 0, true);
								return;
							}
							auto seedexist = std::experimental::filesystem::exists("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (seedexist)
							{
								json j;
								ifstream fs("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								fs >> j;
								fs.close();

								int c = getItemDef(tile).clothType;
								if (c == 0) {
									//world->items.at(x + (y * world->width)).clothHead = tile;
									if (j["clothHead"].get<string>() != "0") return;
									j["clothHead"] = to_string(tile);
								}
								else if (c == 7) {
									//world->items.at(x + (y * world->width)).clothHair = tile;
									if (j["clothHair"].get<string>() != "0") return;
									j["clothHair"] = to_string(tile);
								}
								else if (c == 4) {
									//world->items.at(x + (y * world->width)).clothMask = tile;
									if (j["clothMask"].get<string>() != "0") return;
									j["clothMask"] = to_string(tile);
								}
								else if (c == 8) {
									//world->items.at(x + (y * world->width)).clothNeck = tile;
									if (j["clothNeck"].get<string>() != "0") return;
									j["clothNeck"] = to_string(tile);
								}
								else if (c == 6) {
									//world->items.at(x + (y * world->width)).clothBack = tile;
									if (j["clothBack"].get<string>() != "0") return;
									j["clothBack"] = to_string(tile);
								}
								else if (c == 1) {
									//world->items.at(x + (y * world->width)).clothShirt = tile;
									if (j["clothShirt"].get<string>() != "0") return;
									j["clothShirt"] = to_string(tile);
								}
								else if (c == 2) {
									//world->items.at(x + (y * world->width)).clothPants = tile;
									if (j["clothPants"].get<string>() != "0") return;
									j["clothPants"] = to_string(tile);
								}
								else if (c == 3) {
									//world->items.at(x + (y * world->width)).clothFeet = tile;
									if (j["clothFeet"].get<string>() != "0") return;
									j["clothFeet"] = to_string(tile);
								}
								else if (c == 5) {
									//world->items.at(x + (y * world->width)).clothHand = tile;
									if (j["clothHand"].get<string>() != "0") return;
									j["clothHand"] = to_string(tile);
								}

								if (c != 10) {
									auto iscontains = false;
									SearchInventoryItem(peer, tile, 1, iscontains);
									if (iscontains)
									{
										updateMannequin(peer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).sign,
											atoi(j["clothHair"].get<string>().c_str()), atoi(j["clothHead"].get<string>().c_str()),
											atoi(j["clothMask"].get<string>().c_str()), atoi(j["clothHand"].get<string>().c_str()), atoi(j["clothNeck"].get<string>().c_str()),
											atoi(j["clothShirt"].get<string>().c_str()), atoi(j["clothPants"].get<string>().c_str()), atoi(j["clothFeet"].get<string>().c_str()),
											atoi(j["clothBack"].get<string>().c_str()), true, 0);

										ofstream of("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
										of << j;
										of.close();
										RemoveInventoryItem(tile, 1, peer, true);
									}
								}
							}
						}
					}
				}
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount];++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						bool RotatedRight = false;
						auto xpos = x * 32;
						auto ppos = static_cast<PlayerInfo*>(currentPeer->data)->x;
						if (static_cast<PlayerInfo*>(currentPeer->data)->x < x * 32) RotatedRight = true;
						if (RotatedRight) ppos += 19;
						xpos = xpos / 32;
						ppos = ppos / 32;
						if (ppos == xpos && static_cast<PlayerInfo*>(currentPeer->data)->y / 32 == y && getItemDef(tile).properties != Property_NoSeed && getItemDef(tile).properties != Property_Foreground && getItemDef(tile).properties != Property_MultiFacing && getItemDef(tile).blockType != BlockTypes::SEED && getItemDef(tile).blockType != BlockTypes::STEAM && getItemDef(tile).blockType != BlockTypes::UNKNOWN && getItemDef(tile).blockType != BlockTypes::VENDING && getItemDef(tile).blockType != BlockTypes::ANIM_FOREGROUND && getItemDef(tile).blockType != BlockTypes::BULLETIN_BOARD && getItemDef(tile).blockType != BlockTypes::FACTION && getItemDef(tile).blockType != BlockTypes::CHEST && getItemDef(tile).blockType != BlockTypes::GEMS && getItemDef(tile).blockType != BlockTypes::MAGIC_EGG && getItemDef(tile).blockType != BlockTypes::growplay && getItemDef(tile).blockType != BlockTypes::MAILBOX && getItemDef(tile).blockType != BlockTypes::PORTAL && getItemDef(tile).blockType != BlockTypes::PLATFORM && getItemDef(tile).blockType != BlockTypes::SFX_FOREGROUND && getItemDef(tile).blockType != BlockTypes::CHEMICAL_COMBINER && getItemDef(tile).blockType != BlockTypes::SWITCH_BLOCK && getItemDef(tile).blockType != BlockTypes::TRAMPOLINE && getItemDef(tile).blockType != BlockTypes::TOGGLE_FOREGROUND && getItemDef(tile).blockType != BlockTypes::GROUND_BLOCK && getItemDef(tile).blockType != BlockTypes::BACKGROUND && getItemDef(tile).blockType != BlockTypes::MAIN_DOOR && getItemDef(tile).blockType != BlockTypes::SIGN && getItemDef(tile).blockType != BlockTypes::DOOR && getItemDef(tile).blockType != BlockTypes::CHECKPOINT && getItemDef(tile).blockType != BlockTypes::GATEWAY && getItemDef(tile).blockType != BlockTypes::TREASURE && getItemDef(tile).blockType != BlockTypes::WEATHER) return;
					}
				}
				if (world->owner == "" && getItemDef(tile).blockType == BlockTypes::LOCK && tile != 202 && tile != 204 && tile != 206 && tile != 4994) {
					bool block_place = false;
					string whosowner = "";
					for (int i = 0; i < world->width * world->height; i++) {
						if (world->items.at(i).foreground == 202 && world->items.at(i).monitorname != pData->rawName || world->items.at(i).foreground == 204 && world->items.at(i).monitorname != pData->rawName || world->items.at(i).foreground == 206 && world->items.at(i).monitorname != pData->rawName || world->items.at(i).foreground == 4994 && world->items.at(i).monitorname != pData->rawName) {
							whosowner = world->items.at(i).monitorname;
							block_place = true;
							break;
						}
					}
					if (block_place) {
						try {
							ifstream read_player("save/players/_" + whosowner + ".json");
							if (!read_player.is_open()) {
								return;
							}		
							json j;
							read_player >> j;
							read_player.close();
							string nickname = j["nick"];
							int adminLevel = j["adminLevel"];
							if (nickname == "") {
								nickname = role_prefix.at(adminLevel) + whosowner;
							} 
							Player::OnTalkBubble(peer, pData->netID, "`wThat area is owned by " + nickname + "", 0, false);
						} catch (std::exception& e) {
							std::cout << e.what() << std::endl;
							return;
						}
						return;
					}
				}
				if (getItemDef(tile).blockType == BlockTypes::LOCK) {
					if (world->owner != "") {
						if (!isWorldOwner(peer, world)) {
							try {
								ifstream read_player("save/players/_" + world->owner + ".json");
								if (!read_player.is_open()) {
									return;
								}		
								json j;
								read_player >> j;
								read_player.close();
								string nickname = j["nick"];
								int adminLevel = j["adminLevel"];
								if (nickname == "") {
									nickname = role_prefix.at(adminLevel) + world->owner;
								} 
								Player::OnTalkBubble(peer, pData->netID, "`wThat area is owned by " + nickname + "", 0, false);
								Player::PlayAudio(peer, "audio/locked.wav", 0);
							} catch (std::exception& e) {
								std::cout << e.what() << std::endl;
								return;
							}
						} else {
							Player::OnTalkBubble(peer, pData->netID, "`0Sadece bir tane `$Dunya Kilidi `0dunyaya koylabilir.!", 0, true);
						}
						return;
					}
				}
				if (getItemDef(tile).blockType == BlockTypes::LOCK && world->items.at(x + (y * world->width)).foreground == 0) {
					if (tile == 202 || tile == 204 || tile == 206 || tile == 4994) {
						if (!restricted_area_check(world, x, y)) {
							Player::OnTalkBubble(peer, pData->netID, "Cant place " + getItemDef(tile).name + " here!", 0, false);
							return;
						}
						world->items.at(x + (y * world->width)).monitorname = pData->rawName;
						isSmallLock = true;	
						Player::OnTalkBubble(peer, pData->netID, "Alan kilitlendi.", 0, false);	
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								Player::PlayAudio(currentPeer, "audio/use_lock.wav", 0);
							}
						}	
					} else {
						world->owner = pData->rawName;
						world->isPublic = false;
						pData->worldsowned.push_back(pData->currentWorld);
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								Player::OnConsoleMessage(currentPeer, "`3[`w" + world->name + " `oisimli dunyayi `$Dunyayi `o " + pData->displayName + " aldi `3]");
								Player::OnTalkBubble(currentPeer, pData->netID, "`3[`w" + world->name + " `oisimli dunyayi `$Dunyayi `o " + pData->displayName + " aldi `3]", 0, true);
								Player::PlayAudio(currentPeer, "audio/use_lock.wav", 0);
							}
						}
						if (pData->displayName.find("`") != string::npos) {} else {
							pData->displayName = "`2" + pData->displayName;
							Player::OnNameChanged(peer, pData->netID, pData->displayName);
						}
						isLock = true;
					}
				}
				SyncFish(world, peer);
				if (tile == 2914 && isFishingRod(GetPeerData(peer)) || tile == 3016 && isFishingRod(GetPeerData(peer)))
				{
					if (world->items.at(x + (y * world->width)).water)
					{
						int PlayerPos = round(pData->x / 32);
						int PlayerPosY = round(pData->y / 32);
						if (PlayerPos != x && PlayerPos + 1 != x && PlayerPos - 1 != x || PlayerPosY != y && PlayerPosY + 1 != y)
						{
							Player::OnTalkBubble(peer, pData->netID, "Too far away...", 0, true);
							return;
						}
						if (pData->x != 0 && !pData->Fishing)
						{
							RemoveInventoryItem(tile, 1, peer, true);
							pData->FishPosX = x * 32;
							pData->FishPosY = y * 32;
							pData->Fishing = true;
							pData->LastBait = getItemDef(tile).name;
							SendFishingState(peer);
							for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
								if (isHere(peer, currentPeer)) {
									SendFishing(currentPeer, pData->netID, x, y);
								}
							}
						}
						else
						{
							pData->FishPosX = 0;
							pData->FishPosY = 0;
							pData->Fishing = false;
							send_state(peer);
							Player::OnSetPos(peer, pData->netID, pData->x, pData->y);
						}
					}
					return;
				}
				if (pData->Fishing)
				{
					pData->FishPosX = 0;
					pData->FishPosY = 0;
					pData->Fishing = false;
					send_state(peer);
					Player::OnSetPos(peer, pData->netID, pData->x, pData->y);
					Player::OnTalkBubble(peer, pData->netID, "`wSit perfectly while fishing`w!", 0, true);
					return;
				}
				if (pData->cloth_hand == 3494)
				{
					if (world->owner == "" || pData->rawName == PlayerDB::getProperName(world->owner) || isDev(peer))
					{
						switch (tile)
						{
							case 3478:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).red && !world->items.at(x + (y * world->width)).green && !world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3478, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3479, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = true;
								world->items.at(x + (y * world->width)).green = false;
								world->items.at(x + (y * world->width)).blue = false;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 1953289573, 168, 0);
									}
								}
								return;
							}
							case 3480:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).red && world->items.at(x + (y * world->width)).green && !world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3480, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3481, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = true;
								world->items.at(x + (y * world->width)).green = true;
								world->items.at(x + (y * world->width)).blue = false;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 1153289573, 168, 0);
									}
								}
								return;
							}
							case 3482:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (!world->items.at(x + (y * world->width)).red && world->items.at(x + (y * world->width)).green && !world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3482, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3483, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = false;
								world->items.at(x + (y * world->width)).green = true;
								world->items.at(x + (y * world->width)).blue = false;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 94634864, 168, 0);
									}
								}
								return;
							}
							case 3484:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (!world->items.at(x + (y * world->width)).red && world->items.at(x + (y * world->width)).green && world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3484, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3485, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = false;
								world->items.at(x + (y * world->width)).green = true;
								world->items.at(x + (y * world->width)).blue = true;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 3253289573, 168, 0);
									}
								}
								return;
							}
							case 3486:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (!world->items.at(x + (y * world->width)).red && !world->items.at(x + (y * world->width)).green && world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3486, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3486, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = false;
								world->items.at(x + (y * world->width)).green = false;
								world->items.at(x + (y * world->width)).blue = true;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 2553289573, 168, 0);
									}
								}
								return;
							}
							case 3488:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).red && !world->items.at(x + (y * world->width)).green && world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3488, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3489, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = true;
								world->items.at(x + (y * world->width)).green = false;
								world->items.at(x + (y * world->width)).blue = true;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 3205873253, 168, 0);
									}
								}
								return;
							}
							case 3490:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).red && world->items.at(x + (y * world->width)).green && world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "That block is already painted that color!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3490, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3491, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = true;
								world->items.at(x + (y * world->width)).green = true;
								world->items.at(x + (y * world->width)).blue = true;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, 0, 168, 0);
									}
								}
								return;
							}
							case 3492:
							{
								if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::MAIN_DOOR || getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK || world->items.at(x + (y * world->width)).foreground == 8)
								{
									Player::OnTalkBubble(peer, pData->netID, "That's too special to paint.", 0, false);
									return;
								}
								if (!world->items.at(x + (y * world->width)).red && !world->items.at(x + (y * world->width)).green && !world->items.at(x + (y * world->width)).blue) {
									Player::OnTalkBubble(peer, pData->netID, "Don't waste your varnish on an unpainted block!", 0, false);
									return;
								}
								if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background == 0) {
									Player::OnTalkBubble(peer, pData->netID, "There's nothing to paint!", 0, false);
									return;
								}
								RemoveInventoryItem(3492, 1, peer, true);
								if (world->items.at(x + (y * world->width)).foreground == 0 || isSeed(world->items.at(x + (y * world->width)).foreground)) {
									if (rand() % 100 <= 15) {
										DropItem(world, peer, -1, data.punchX * 32 + rand() % 18, data.punchY * 32 + rand() % 18, 3493, 1, 0);
									}
									else if (rand() % 100 <= 35) {
										SendFarmableDrop(peer, 5, data.punchX, data.punchY, world);
									}
								}
								world->items.at(x + (y * world->width)).red = false;
								world->items.at(x + (y * world->width)).green = false;
								world->items.at(x + (y * world->width)).blue = false;
								UpdateVisualsForBlock(peer, true, x, y, world);
								for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
									if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, net_peer)) {
										SendParticleEffect(net_peer, x * 32 + 16, y * 32 + 16, -1, 168, 0);
									}
								}
								return;
							}
							default: break;
						}
					}
				}
				bool hassmallock = false;
				for (int i = 0; i < world->width * world->height; i++) {
					if (world->items.at(i).foreground == 202 || world->items.at(i).foreground == 204 || world->items.at(i).foreground == 206 || world->items.at(i).foreground == 4994) {
						hassmallock = true;
						break;
					}
				}
				if (hassmallock && !isDev(peer) || world->owner != "" && !isWorldOwner(peer, world) && !isWorldAdmin(peer, world) && !isDev(peer)) {
					if (pData->rawName == world->owner || isDev(peer) || tile == world->publicBlock || causedBy == -1 || tile == 5640 || hassmallock && !restricted_area(peer, world, x, y)) {

					}
					else if (isWorldAdmin(peer, world)) {
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
							try {
								ifstream read_player("save/players/_" + world->owner + ".json");
								if (!read_player.is_open()) {
									return;
								}		
								json j;
								read_player >> j;
								read_player.close();
								string nickname = j["nick"];
								int adminLevel = j["adminLevel"];
								if (nickname == "") {
									nickname = role_prefix.at(adminLevel) + world->owner;
								} 
								Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`2YETKI SAGLANDI`w)", 0, true);
								Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
							} catch (std::exception& e) {
								std::cout << e.what() << std::endl;
								return;
							}
							return;
						}
					} else if (world->isPublic) {
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
							try {
								ifstream read_player("save/players/_" + world->owner + ".json");
								if (!read_player.is_open()) {
									return;
								}		
								json j;
								read_player >> j;
								read_player.close();
								string nickname = j["nick"];
								int adminLevel = j["adminLevel"];
								if (nickname == "") {
									nickname = role_prefix.at(adminLevel) + world->owner;
								} 
								Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKES SERBEST`w)", 0, true);
								Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
							} catch (std::exception& e) {
								std::cout << e.what() << std::endl;
								return;
							}
							return;
						}
					} else if (world->isEvent) {
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
							string whoslock = world->owner;
							if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
								whoslock = world->items.at(x + (y * world->width)).monitorname;
							}
							if (pData->rawName != whoslock) {
								try {
									ifstream read_player("save/players/_" + whoslock + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + whoslock;
									} 
									if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
									else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4YETKI SAGLANDI`w)", 0, true);
									Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
								return;
							}
						} else if (world->items.at(x + (y * world->width)).foreground != world->publicBlock && causedBy != -1) {
							Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
							return;
						}
					} else {
						Player::PlayAudio(peer, "audio/punch_locked.wav", 0);
						return;
					}
					if (tile == 18 && isDev(peer)) {
						if (isWorldAdmin(peer, world) && !isWorldOwner(peer, world)) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`2YETKI SAGLANDI`w)", 0, true);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
							}
						} else if (world->isPublic && !isWorldOwner(peer, world) || world->items.at(x + (y * world->width)).foreground == 202 && world->items.at(x + (y * world->width)).opened || world->items.at(x + (y * world->width)).foreground == 204 && world->items.at(x + (y * world->width)).opened || world->items.at(x + (y * world->width)).foreground == 206 && world->items.at(x + (y * world->width)).opened || world->items.at(x + (y * world->width)).foreground == 4994 && world->items.at(x + (y * world->width)).opened) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								try {
									ifstream read_player("save/players/_" + world->owner + ".json");
									if (!read_player.is_open()) {
										return;
									}		
									json j;
									read_player >> j;
									read_player.close();
									string nickname = j["nick"];
									int adminLevel = j["adminLevel"];
									if (nickname == "") {
										nickname = role_prefix.at(adminLevel) + world->owner;
									} 
									Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
								} catch (std::exception& e) {
									std::cout << e.what() << std::endl;
									return;
								}
							}
						} else if (world->isEvent && !isWorldOwner(peer, world) || world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								string whoslock = world->owner;
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									whoslock = world->items.at(x + (y * world->width)).monitorname;
								}
								if (pData->rawName != whoslock) {
									try {
										ifstream read_player("save/players/_" + whoslock + ".json");
										if (!read_player.is_open()) {
											return;
										}		
										json j;
										read_player >> j;
										read_player.close();
										string nickname = j["nick"];
										int adminLevel = j["adminLevel"];
										if (nickname == "") {
											nickname = role_prefix.at(adminLevel) + whoslock;
										} 
										if (world->items.at(x + (y * world->width)).opened) Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`9HERKESE ACIK`w)", 0, true);
										else Player::OnTalkBubble(peer, pData->netID, "`w" + nickname + "'s `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "`w. (`4YETKI YOK`w)", 0, true);
									} catch (std::exception& e) {
										std::cout << e.what() << std::endl;
										return;
									}
								}
							}
						}
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 10 && tile == 3400)
				{
					if (pData->rawName == world->owner || isDev(peer) || world->owner == "")
					{
						RemoveInventoryItem(3400, 1, peer, true);
						world->items.at(x + (y * world->width)).foreground = 392;
						PlayerMoving data3{};
						data3.packetType = 0x3;
						data3.characterState = 0x0;
						data3.x = x;
						data3.y = y;
						data3.punchX = x;
						data3.punchY = y;
						data3.XSpeed = 0;
						data3.YSpeed = 0;
						data3.netID = -1;
						data3.plantingTree = 392;
						ENetPeer* currentPeer;
						for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
						{
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer))
							{
								auto raw = packPlayerMoving(&data3);
								raw[2] = dicenr;
								raw[3] = dicenr;
								SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
								Player::OnParticleEffect(currentPeer, 44, x * 32, y * 32, 0);
							}
						}
					}
				}

				if (world->items.at(x + (y * world->width)).foreground == 2946 && tile != 18 && tile != 32 && tile > 0)
				{
					if (pData->rawName == world->owner || isDev(peer))
					{
						if (pData->lastDISPLAY + 1000 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
						{
							pData->lastDISPLAY = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
							auto iscontains = false;
							SearchInventoryItem(peer, tile, 1, iscontains);
							if (!iscontains) return;
							else
							{
								auto xSize = world->width;
								auto ySize = world->height;
									auto n = tile;
									if (getItemDef(n).properties & Property_Untradable || n == 6336 || n == 8552 || n == 9472 || n == 5640 || n == 9482 || n == 9356 || n == 9492 || n == 9498 || n == 8774 || n == 1790 || n == 2592 || n == 1784 || n == 1792 || n == 1794 || n == 7734 || n == 8306 || n == 9458)
									{
										Player::OnTalkBubble(peer, pData->netID, "You can't display untradeable items.", 0, true);
										return;
									}
									if (getItemDef(n).blockType == BlockTypes::LOCK || n == 2946)
									{
										Player::OnTalkBubble(peer, pData->netID, "Sorry, no displaying Display Blocks or Locks.", 0, true);
										return;
									}
									if (world->items.at(x + (y * world->width)).intdata == 0)
									{
										world->items.at(x + (y * world->width)).intdata = tile;
										ENetPeer* currentPeer;
										for (currentPeer = server->peers;
											currentPeer < &server->peers[server->peerCount];
											++currentPeer)
										{
											if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
											if (isHere(peer, currentPeer))
											{
												UpdateVisualsForBlock(currentPeer, true, x, y, world);
												SendThrowEffect(currentPeer, tile, pData->netID, -1, 150, 0, x * 32 + 16, y * 32 + 16);
											}
										}
										RemoveInventoryItem(n, 1, peer, true);
										updateplayerset(peer, n);
									}
									else
									{
										Player::OnTalkBubble(peer, pData->netID, "Remove what's in there first!", 0, true);
									}
									return;
							}
						}
						else
						{
							Player::OnTalkBubble(peer, pData->netID, "Slow down while using display blocks!", 0, true);;
							return;
						}
					}
					else
					{
						if (world->owner == "")
						{
							Player::OnTalkBubble(peer, pData->netID, "This area must be locked to put your item on display!", 0, true);
						}
						else if (world->isPublic)
						{
							Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
						}
						else
						{
							Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
						}
						return;
					}
				}
				if (world->items.at(x + (y * world->width)).foreground == 3528 && tile != 18 && tile != 32 && tile > 0) {
					if (pData->rawName == world->owner || isDev(peer)) {
						auto n = tile;
						world->items.at(x + (y * world->width)).intdata = tile;
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								send_canvas_data(peer, world->items.at(x + (y * world->width)).foreground, world->items.at(x + (y * world->width)).background, x, y, tile, getItemDef(tile).name);
								SendThrowEffect(currentPeer, tile, pData->netID, -1, 150, 0, x * 32 + 16, y * 32 + 16);
							}
						}
						return;
					} else {
						if (world->owner == "")
						{
							Player::OnTalkBubble(peer, pData->netID, "This area must be locked to put your item on display!", 0, true);
						}
						else if (world->isPublic)
						{
							Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
						}
						else
						{
							Player::OnTalkBubble(peer, pData->netID, "That area is owned by " + world->owner + "", 0, true);
						}
						return;
					}
				}
				if (getItemDef(tile).blockType == BlockTypes::CONSUMABLE || getItemDef(tile).blockType == BlockTypes::CLOTHING) return;		
				break;
			}
		}
		if (causedBy != -1)
		{
			if (!isDev(peer))
			{
				auto iscontains = false;
				SearchInventoryItem(peer, tile, 1, iscontains);
				if (!iscontains) return;
			}
		}
		ENetPeer* currentPeer;
		bool Explosion = false;
		if (tile == 18) {
			if (pData->cloth_hand == 3066 && tile == 18) return;
			if (world->items.at(x + (y * world->width)).background == 6864 && world->items.at(x + (y * world->width)).foreground == 0) return;
			if (world->items.at(x + (y * world->width)).background == 0 && world->items.at(x + (y * world->width)).foreground == 0) return;
			if (world->items.at(x + (y * world->width)).fire) return;
			ItemDefinition brak;
			if (world->items.at(x + (y * world->width)).foreground != 0) {
				brak = getItemDef(world->items.at(x + (y * world->width)).foreground);			
			} else {
				brak = getItemDef(world->items.at(x + (y * world->width)).background);
			}
			bool block_one_hit = false;
			if (world->width == 90 && world->height == 60 || world->width == 90 && world->height == 110) block_one_hit = true;
			data.packetType = 0x8;
			data.plantingTree = 6;
			if (break_effect.find(pData->cloth_necklace) != break_effect.end() || break_effect.find(pData->cloth_hand) != break_effect.end() || break_effect.find(pData->cloth_back) != break_effect.end()) {
				if (isWorldOwner(peer, world) || isDev(peer) || world->isPublic || world->owner == "") {
					if (!block_one_hit) {
						int p_break_effect = 0;
						if (break_effect.find(pData->cloth_hand) != break_effect.end()) {
							p_break_effect = break_effect.at(pData->cloth_hand);
						}
						if (p_break_effect == 0 && break_effect.find(pData->cloth_back) != break_effect.end()) {
							p_break_effect = break_effect.at(pData->cloth_back);
						}
						if (p_break_effect == 0 && break_effect.find(pData->cloth_necklace) != break_effect.end()) {
							p_break_effect = break_effect.at(pData->cloth_necklace);
						}
						if (p_break_effect != 0) {
							int particle_resize = 0;
							switch (p_break_effect) { /*recode this later*/
								case 97:
								{
									int kek = world->items.at(x + (y * world->width)).foreground;
									if (world->items.at(x + (y * world->width)).foreground != 0) kek = world->items.at(x + (y * world->width)).foreground;
									else kek = world->items.at(x + (y * world->width)).background;
									particle_resize = kek;
									break;
								}
								default:
								{
									break;
								}
							} for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
								if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
								if (isHere(peer, net_peer)) {
									SendParticleEffect(net_peer, x * 32 + 16, y * 32, particle_resize, p_break_effect, 0);
								}
							}
						}
					}
				}
			}
			bool no_fire = false;
			bool break_background = false;
			if (tile == 18 && world->items.at(x + (y * world->width)).foreground == 9368 || explosive.find(pData->cloth_hand) != explosive.end() || explosive.find(pData->cloth_back) != explosive.end() || explosive.find(pData->cloth_necklace) != explosive.end()) { /*explosives*/
				if (isWorldOwner(peer, world) || isDev(peer) || world->isPublic || world->owner == "") {
					Explosion = true;
					if (explosive.find(pData->cloth_hand) != explosive.end() || explosive.find(pData->cloth_back) != explosive.end() || explosive.find(pData->cloth_necklace) != explosive.end()) {
						break_background = true;
						if (world->width == 90 && world->height == 60) break_background = false;
						no_fire = true;
					}
					int kek = world->items.at(x + (y * world->width)).foreground;
					if (world->items.at(x + (y * world->width)).foreground != 0) kek = world->items.at(x + (y * world->width)).foreground;
					else kek = world->items.at(x + (y * world->width)).background;
					for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
						if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
						if (isHere(peer, net_peer)) {
							SendParticleEffect(net_peer, x * 32, y * 32, kek, 97, 0);
							Player::OnParticleEffect(net_peer, 43, (x + 1) * 32, y * 32, 0);
							Player::OnParticleEffect(net_peer, 43, (x - 1) * 32, y * 32, 0);
							Player::OnParticleEffect(net_peer, 43, x * 32, (y + 1) * 32, 0);
							Player::OnParticleEffect(net_peer, 43, x * 32, (y - 1) * 32, 0);
							Player::OnParticleEffect(net_peer, 43, (x + 1) * 32, (y + 1) * 32, 0);
							Player::OnParticleEffect(net_peer, 43, (x - 1) * 32, (y - 1) * 32, 0);
							Player::OnParticleEffect(net_peer, 43, (x + 1) * 32, (y - 1) * 32, 0);
							Player::OnParticleEffect(net_peer, 43, (x - 1) * 32, (y + 1) * 32, 0);
						}
					}
				}
			}
			bool EarthMastery = false;
			int ChanceOfEarth = 0;
			if (pData->level >= 4) ChanceOfEarth = 1;
			if (pData->level >= 6) ChanceOfEarth = 2;
			if (pData->level >= 8) ChanceOfEarth = 3;
			if (world->items.at(x + (y * world->width)).foreground == 2 && !block_one_hit && tile == 18 && rand() % 100 <= ChanceOfEarth && pData->level >= 4) {
				if (isWorldOwner(peer, world) || isDev(peer) || world->isPublic || world->owner == "") {
					EarthMastery = true;
					int kek = world->items.at(x + (y * world->width)).foreground;
					if (world->items.at(x + (y * world->width)).foreground != 0) kek = world->items.at(x + (y * world->width)).foreground;
					else kek = world->items.at(x + (y * world->width)).background;
					for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
						if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
						if (isHere(peer, net_peer)) {
							SendParticleEffect(net_peer, x * 32 + 16, y * 32, kek, 97, 0);
						}
					}
				}
			}
			if (pData->PunchPotion && tile == 18 && !block_one_hit || pData->cloth_necklace == 6260 && tile == 18 && !block_one_hit || pData->cloth_hand == 9164 && tile == 18 && !block_one_hit || pData->cloth_hand == 9488 && tile == 18 && !block_one_hit || pData->cloth_hand == 9496 && tile == 18 && !block_one_hit) {
				if (isWorldOwner(peer, world) || isDev(peer) || world->isPublic || world->owner == "") {
					int kek = world->items.at(x + (y * world->width)).foreground;
					if (world->items.at(x + (y * world->width)).foreground != 0) kek = world->items.at(x + (y * world->width)).foreground;
					else kek = world->items.at(x + (y * world->width)).background;
					for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
						if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
						if (isHere(peer, net_peer)) {
							SendParticleEffect(net_peer, x * 32 + 16, y * 32, kek, 97, 0);
						}
					}
				}
			}
			if ((duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && !Explosion && !EarthMastery && !pData->PunchPotion && one_hit.find(pData->cloth_necklace) == one_hit.end() && one_hit.find(pData->cloth_back) == one_hit.end() && one_hit.find(pData->cloth_hand) == one_hit.end() || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && one_hit.find(pData->cloth_necklace) != one_hit.end() && one_hit.find(pData->cloth_back) != one_hit.end() && one_hit.find(pData->cloth_hand) != one_hit.end() && !Explosion && !EarthMastery || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && pData->cloth_hand == 9496 && !Explosion && !EarthMastery || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && pData->cloth_hand == 9488 && !Explosion && !EarthMastery || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && pData->cloth_hand == 9164 && !Explosion && !EarthMastery || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && pData->cloth_necklace == 6260 && !Explosion && !EarthMastery || (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count() - world->items.at(x + (y * world->width)).breakTime >= 4000 && block_one_hit && world->width == 90 && world->height == 60 && pData->PunchPotion && !Explosion && !EarthMastery) {
				world->items.at(x + (y * world->width)).breakTime = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
				world->items.at(x + (y * world->width)).breakLevel = 4;
			} else { 
				if (isPickaxe(GetPeerData(peer))) brak.breakHits -= 1;
				int break_hits = brak.breakHits;
				if (world->width == 90 && world->height == 60) {
					int white_door_level = 0;
					for (int i = 0; i < world->width * world->height; i++) {
						if (getItemDef(world->items.at(i).foreground).blockType == BlockTypes::MAIN_DOOR) {
							white_door_level = i + 100;
							break;
						}
					}
					int deep = 0;
					if (white_door_level < (x + (y * world->width))) deep = (x + (y * world->width - white_door_level)) / 100;
					break_hits += deep;
				}
				if (y < world->height && world->items.at(x + (y * world->width)).breakLevel + 4 >= break_hits * 4 || one_hit.find(pData->cloth_necklace) != one_hit.end() && tile == 18 && !block_one_hit || one_hit.find(pData->cloth_back) != one_hit.end() && tile == 18 && !block_one_hit || one_hit.find(pData->cloth_hand) != one_hit.end() && tile == 18 && !block_one_hit) {
				data.packetType = 0x3;
				data.netID = causedBy;
				data.plantingTree = 18;
				data.punchX = x;
				data.punchY = y;
				world->items.at(x + (y * world->width)).breakLevel = 0;
				auto hi = data.punchX * 32;
				auto hi2 = data.punchY * 32;
				int repeat = 1;
				if (Explosion) {
					repeat = 9;
				}
				int o_x = x;
				int o_y = y;
				for (int i = 1; i <= repeat; i++) {
					if (Explosion) { /*2x2 radius break*/
						int x_r = o_x - 1;
						int y_r = o_y - 1;
						if (i < 3) x_r += i;
						if (i > 3 && i <= 6) {
							x_r += i - 4;
							y_r += 1;
						}
						if (i > 6 && i <= 9) {
							x_r += i - 7;
							y_r += 2;
						}
						x = x_r;
						y = y_r;
					}
					if (x < 0 || y < 0 || x > world->width - 1 || y > world->height - 1) continue;
					if (Explosion && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) continue;
					if (Explosion && world->items.at(x + (y * world->width)).foreground != 8 && world->items.at(x + (y * world->width)).foreground != 0 && getItemDef(world->items.at(x + (y * world->width)).foreground).properties & Property_Mod) continue;
					if (world->items.at(x + (y * world->width)).foreground == 0 && world->items.at(x + (y * world->width)).background != 0 && Explosion && !break_background) continue;
					if (world->items.at(x + (y * world->width)).foreground == 8 && Explosion && rand() % 100 >= 5) continue;
					if (world->items.at(x + (y * world->width)).foreground == 8 && y >= world->height - 1) continue;
					if (Explosion && rand() % 100 <= 5 && world->items.at(x + (y * world->width)).background != 0 && !no_fire) { /*fire*/
						if (world->items.at(x + (y * world->width)).foreground != 3528 && !world->items.at(x + (y * world->width)).water && !world->items.at(x + (y * world->width)).fire && !isSeed(world->items.at(x + (y * world->width)).foreground) && world->items.at(x + (y * world->width)).foreground != 8 && world->items.at(x + (y * world->width)).foreground != 6 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::LOCK && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::DISPLAY && world->items.at(x + (y * world->width)).foreground != 6952 && world->items.at(x + (y * world->width)).foreground != 6954 && world->items.at(x + (y * world->width)).foreground != 5638 && world->items.at(x + (y * world->width)).foreground != 6946 && world->items.at(x + (y * world->width)).foreground != 6948 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::VENDING && world->items.at(x + (y * world->width)).foreground != 1420 && world->items.at(x + (y * world->width)).foreground != 6214 && world->items.at(x + (y * world->width)).foreground != 1006 && world->items.at(x + (y * world->width)).foreground != 656 && world->items.at(x + (y * world->width)).foreground != 1420 && getItemDef(world->items.at(x + (y * world->width)).foreground).blockType != BlockTypes::DONATION) {
							world->items.at(x + (y * world->width)).fire = true;
							UpdateVisualsForBlock(peer, true, x, y, world);
						}
					}
					if (Explosion) {
						PlayerMoving data3{};        
 						data3.packetType = 0x3;
						data3.characterState = 0x0;
						data3.x = x;
						data3.y = y;
						data3.punchX = x;
						data3.punchY = y;
						data3.XSpeed = 0;
						data3.YSpeed = 0;
						data3.netID = causedBy;
						data3.plantingTree = tile;
						for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								auto raw = packPlayerMoving(&data3);
								raw[2] = dicenr;
								raw[3] = dicenr;
								SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
							}
						}
					}
					if (world->items.at(x + (y * world->width)).foreground == 5140) {
						std::vector<int> list{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4426 };
						int index = rand() % list.size();
						auto value = list[index];
						if (value == 4426) {
							//srand(GetTickCount());
							auto droploc = rand() % 18;
							auto droplocs = rand() % 18;
							DropItem(world, peer, -1, x * 32 + droploc, y * 32 + droplocs, 4426, 1, 0);
						}
					}
					else if (world->items.at(x + (y * world->width)).foreground == 3402 || world->items.at(x + (y * world->width)).foreground == 392 || world->items.at(x + (y * world->width)).foreground == 9350) {
						if (world->items.at(x + (y * world->width)).foreground == 3402) {
							pData->bootybreaken++;
							if (pData->bootybreaken >= 100) {
								gamepacket_t p;
								p.Insert("OnProgressUIUpdateValue");
								p.Insert(100);
								p.Insert(0);
								p.CreatePacket(peer);
							} else {
								gamepacket_t p;
								p.Insert("OnProgressUIUpdateValue");
								p.Insert(pData->bootybreaken);
								p.Insert(0);
								p.CreatePacket(peer);
							}
						}
						vector<int> list{ 362, 3398, 386, 4422, 364, 9340, 9342, 9332, 9334, 9336, 9338, 366, 2388, 7808, 7810, 4416, 7818, 7820, 5652, 7822, 7824, 5644, 390, 7826, 7830, 9324, 5658, 3396, 2384, 5660, 3400, 4418, 4412, 388, 3408, 1470, 3404, 3406, 2390, 5656, 5648, 2396, 384, 5664, 4424, 4400, 1458, 10660, 10654, 10632, 10652, 10626, 10640, 10662 };
						int itemid = list[rand() % list.size()];
						if (itemid == 1458) { 
							int target = 5;
							if (world->items.at(x + (y * world->width)).foreground == 9350) target = 20;
							if ((rand() % 1000) <= target) { }
							else itemid = 7808;
						}
						
						DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, itemid, 1, 0);
					}
					else if (world->items.at(x + (y * world->width)).foreground == 10004) {
						vector<int> list{ 3764, 3702, 9750, 3700, 842, 9754, 9758, 2874, 8614, 9730, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3698, 3698, 3698, 1670, 1680, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754, 3702, 3702, 3702, 9750, 9750, 9750, 9750, 9750, 3700, 3700, 3700, 3700, 842, 842, 9754, 9754, 9754 };
						int itemid = list[rand() % list.size()];
						if (itemid == 1674) {
							int target = 5;
							if (world->items.at(x + (y * world->width)).foreground == 3782) target = 20;
							if ((rand() % 1000) <= target) {}
							else itemid = 7808;
						}

						DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, itemid, 1, 0);
					}
					else if (world->items.at(x + (y * world->width)).foreground == 836) {
						vector<int> list{ 442,850,834,822,846,838,840,844,842,848,832,2856,2862,2860,2858,3742,3704,3740,3702,3698,3700,4812,4810,4804,6326,6328,6324,6330,6308,8612,8610 };
						for (int i = 0; i < (rand() % (4 - 2 + 1) + 2); i++)
						{
							int itemid = list[rand() % list.size()];
							DropItem(world, peer, -1, x * 32 + rand() % 96, y * 32 + rand() % 96, itemid, 1, 0);
						}
					}
					else if (pData->cloth_hand == 8452) {
						Player::OnParticleEffect(peer, 149, data.punchX * 32, data.punchY * 32, 0);
					}
					if (world->items.at(x + (y * world->width)).foreground != 0) {
						int custom_drop = 0;
						if (custom_gem_block.find(world->items.at(x + (y * world->width)).foreground) != custom_gem_block.end()) {
							custom_drop = (rand() % custom_gem_block.at(world->items.at(x + (y * world->width)).foreground)) + 1;
						} if (custom_drop_block.find(world->items.at(x + (y * world->width)).foreground) != custom_drop_block.end()) {
							int chance_of_drop = custom_drop_chance.at(world->items.at(x + (y * world->width)).foreground);
							if (((rand() % 100) + 1) <= chance_of_drop) {
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, custom_drop_block.at(world->items.at(x + (y * world->width)).foreground), 1, 0);
							}
						}
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DOOR) {
							if (isWorldOwner(peer, world) || world->owner == "" || isDev(peer)) {
								world->items.at(x + (y * world->width)).label = "";
								world->items.at(x + (y * world->width)).destWorld = "";
								world->items.at(x + (y * world->width)).destId = "";			
								world->items.at(x + (y * world->width)).currId = "";
								world->items.at(x + (y * world->width)).password = "";
							}
						}
						else if (world->items.at(x + (y * world->width)).foreground == 1538 || world->items.at(x + (y * world->width)).foreground == 944 || world->items.at(x + (y * world->width)).foreground == 3930)
						{
							if (rand() % 100 >= 50) {
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, world->items.at(x + (y * world->width)).foreground, 1, 0);
							}
						}
						else if (world->items.at(x + (y * world->width)).foreground == 16)
						{
							if (rand() % 100 >= 85)
							{
								int ingredients[15] = { 4586, 962, 4564, 4570, 868, 196, 4766, 4568, 3472, 7672, 4764, 676, 874, 4666, 4602 };
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), ingredients[rand() % 15], 1, 0);
							}
							else if(rand() % 300 + 1 == 5)
							{
								DropItem(world, peer, -1, x * 32 + (rand() % 16), y * 32 + (rand() % 16), 9530, 1, 0);
							}

						}
						else if (world->items.at(x + (y * world->width)).foreground == 8430) {
							int itemid = rand() % maxItems;
							if (getItemDef(itemid).name.find("null_item") != string::npos) itemid = 5142;
							if (getItemDef(itemid).name.find("Subscription") != string::npos) itemid = 5142;
							if (getItemDef(itemid).name.find("Golden") != string::npos && rand() % 100 >= 10) itemid = 5142;
							if (getItemDef(itemid).name.find("Legendary") != string::npos && rand() % 100 >= 10) itemid = 5142;
							if (itemid == 1458 || itemid == 9506 || itemid == 9510) itemid = 5142;
							if (itemid % 2 == 0) {
								Player::OnTalkBubble(peer, pData->netID, "Dark Stone'nin Gucu Size " + getItemDef(itemid).name + " verdi!", 0, true);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, itemid, 1, 0);
								int x1 = data.punchX * 32;
								int y1 = data.punchY * 32;
								for (ENetPeer* peer2 = server->peers; peer2 < &server->peers[server->peerCount]; ++peer2) {
									if (peer2->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, peer2)) {
										Player::OnParticleEffect(peer2, 182, x1, y1, 0);
									}
								}
							} else {
								itemid += 1;
								Player::OnTalkBubble(peer, pData->netID, "Dark Stone'nin Gucu Size " + getItemDef(itemid).name + "verdi!", 0, true);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, itemid, 1, 0);
								for (ENetPeer* peer2 = server->peers; peer2 < &server->peers[server->peerCount]; ++peer2) {
									if (peer2->state != ENET_PEER_STATE_CONNECTED) continue;
									if (isHere(peer, peer2)) {
										Player::OnParticleEffect(peer2, 182, data.punchX * 32, data.punchY * 32, 0);
									}
								}
							}
						}
						else if (world->items.at(x + (y * world->width)).foreground == 8440) {
							int kiek = 0;
							if (pData->cloth_hand == 98 || Explosion) {
								kiek = 1;
							} if (kiek != 0) {
								SendXP(peer, 5);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 7960, kiek, 0);
							}
							SendFarmableDrop(peer, 10, x, y, world);
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 8532) {
							int kiek = 0;
							if (pData->cloth_hand == 98 || Explosion) {
								kiek = 1;
							} if (kiek != 0) {
								SendXP(peer, 7);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 8428, kiek, 0);
							}
							SendFarmableDrop(peer, 25, x, y, world);
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 9146) {
							int kiek = 0;
							if (pData->cloth_hand == 98 || Explosion) {
								kiek = 1;
							} if (kiek != 0) {
								SendXP(peer, 10);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 8430, kiek, 0);
							}
							SendFarmableDrop(peer, 50, x, y, world);
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 8530) {
							int kiek = 0;
							if (pData->cloth_hand == 98 || Explosion) {
								kiek = 1;
							} if (kiek != 0) {
								SendXP(peer, 15);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 7962, kiek, 0);
							}
							SendFarmableDrop(peer, 100, x, y, world);
						}
						else if (world->items.at(x + (y * world->width)).foreground == 5160) {
							if (pData->cloth_hand == 3932) {
								if ((rand() % 100) <= 50) {
									Player::OnTalkBubble(peer, pData->netID, "`wThe Rock was so hard to break that it broke your Rock Hammer!", 0, true);
									RemoveInventoryItem(3932, 1, peer, true);
									auto iscontains = false;
									SearchInventoryItem(peer, 3932, 1, iscontains);
									if (!iscontains) {
										pData->cloth_hand = 0;
										Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
										pData->effect = 8421376;
										sendPuncheffect(peer, pData->effect);
										send_state(peer); //here
										sendClothes(peer);
									}
								}
								SendXP(peer, 30);
								SendParticleEffect(peer, x * 32 + 16, y * 32 + 16, 3, 114, 0);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 5190, 1, 0);
							}
							SendFarmableDrop(peer, 5, x, y, world);
						}
						else if (world->items.at(x + (y * world->width)).foreground == 5172) {
							if (pData->cloth_hand == 3932) {
								if ((rand() % 100) <= 50) {
									Player::OnTalkBubble(peer, pData->netID, "`wThe Rock was so hard to break that it broke your Rock Hammer!", 0, true);
									RemoveInventoryItem(3932, 1, peer, true);
									auto iscontains = false;
									SearchInventoryItem(peer, 3932, 1, iscontains);
									if (!iscontains) {
										pData->cloth_hand = 0;
										Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
										pData->effect = 8421376;
										sendPuncheffect(peer, pData->effect);
										send_state(peer); //here
										sendClothes(peer);
									}
								}
								SendXP(peer, 30);
								SendParticleEffect(peer, x * 32 + 16, y * 32 + 16, 3, 114, 0);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 5192, 1, 0);
							}
							SendFarmableDrop(peer, 5, x, y, world);
						}
						else if (world->items.at(x + (y * world->width)).foreground == 5174) {
							if (pData->cloth_hand == 3932) {
								if ((rand() % 100) <= 50) {
									Player::OnTalkBubble(peer, pData->netID, "`wThe Rock was so hard to break that it broke your Rock Hammer!", 0, true);
									RemoveInventoryItem(3932, 1, peer, true);
									auto iscontains = false;
									SearchInventoryItem(peer, 3932, 1, iscontains);
									if (!iscontains) {
										pData->cloth_hand = 0;
										Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
										pData->effect = 8421376;
										sendPuncheffect(peer, pData->effect);
										send_state(peer); //here
										sendClothes(peer);
									}
								}
								SendXP(peer, 30);
								SendParticleEffect(peer, x * 32 + 16, y * 32 + 16, 3, 114, 0);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 5194, 1, 0);
							}
							SendFarmableDrop(peer, 5, x, y, world);
							world->items.at(x + (y * world->width)).fire = true;
							if (y + 1 <= 100) world->items.at(x + 1 + (y * world->width)).fire = true;
							if (x - 1 >= 0) world->items.at(x - 1 + (y * world->width)).fire = true;
							if (y - 1 >= 0) world->items.at(x + (y * world->width - 100)).fire = true;
							if (y + 1 <= 60) world->items.at(x + (y * world->width + 100)).fire = true;
							UpdateVisualsForBlock(peer, true, x, y, world);
							if (y + 1 <= 100) UpdateVisualsForBlock(peer, true, x + 1, y, world);
							if (x - 1 >= 0) UpdateVisualsForBlock(peer, true, x - 1, y, world);
							if (y - 1 >= 0) UpdateVisualsForBlock(peer, true, x, y - 1, world);
							if (y + 1 <= 60) UpdateVisualsForBlock(peer, true, x, y + 1, world);
						}
						else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::CHECKPOINT) {
							if (pData->checkx / 32 == x && pData->checky / 32 == y) {
								pData->checkx = 0;
								pData->checky = 0;
								pData->ischeck = false;
							}
						}
						else if (world->items.at(x + (y * world->width)).foreground == 5176) {
							if (pData->cloth_hand == 3932) {
								if ((rand() % 100) <= 50) {
									Player::OnTalkBubble(peer, pData->netID, "`wThe Rock was so hard to break that it broke your Rock Hammer!", 0, true);
									RemoveInventoryItem(3932, 1, peer, true);
									auto iscontains = false;
									SearchInventoryItem(peer, 3932, 1, iscontains);
									if (!iscontains) {
										pData->cloth_hand = 0;
										Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
										pData->effect = 8421376;
										sendPuncheffect(peer, pData->effect);
										send_state(peer); //here
										sendClothes(peer);
									}
								}
								SendXP(peer, 30);
								SendParticleEffect(peer, x * 32 + 16, y * 32 + 16, 3, 114, 0);
								DropItem(world, peer, -1, x * 32 + rand() % 18, y * 32 + rand() % 18, 5178, 1, 0);
							}
							SendFarmableDrop(peer, 5, x, y, world);
						}
						else if (world->items.at(x + (y * world->width)).foreground == 1420 || world->items.at(x + (y * world->width)).foreground == 6214) {
							auto squaresign = x + (y * world->width);
							auto ismannequin = std::experimental::filesystem::exists("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (ismannequin) {
								remove(("save/mannequin/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
							}
							world->items.at(x + (y * world->width)).sign = "";
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 656) {
							auto squaresign = x + (y * world->width);
							auto isdbox = std::experimental::filesystem::exists("save/mailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (isdbox) {
								ifstream ifff("save/mailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								json j;
								ifff >> j;
								ifff.close();
								int count = j["inmail"];
								if (j["inmail"] > 0) {
									Player::OnTextOverlay(peer, "`wThere are `5" + to_string(count) + " `wletter(s) in the mailbox.");
								}
								remove(("save/mailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
							}
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 658) {
							auto squaresign = x + (y * world->width);
							auto isdbox = std::experimental::filesystem::exists("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (isdbox) {
								remove(("save/bulletinboard/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
							}
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 1006) {
							auto squaresign = x + (y * world->width);
							auto isdbox = std::experimental::filesystem::exists("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (isdbox) {
								ifstream ifff("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								json j;
								ifff >> j;
								ifff.close();
								int count = j["inmail"];
								if (j["inmail"] > 0) {
									Player::OnTextOverlay(peer, "`wThere are `5" + to_string(count) + " `wletter(s) in the mailbox.");
								}
								remove(("save/bluemailbox/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
							}
						}
						else if (world->items[x + (y * world->width)].foreground == 1436)
						{
							if (isWorldOwner(peer, world) || world->owner == "")
							{
								remove(("save/securitycam/logs/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".txt").c_str());
								bool success = true;
								SaveItemMoreTimes(1436, 1, peer, success);
							}
							else
							{
								Player::OnTextOverlay(peer, "only world owner can break security camera!");
								return;
							}
						}
						else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::DONATION) {
							auto squaresign = x + (y * world->width);
							auto isdbox = std::experimental::filesystem::exists("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
							if (isdbox) {
								ifstream ifff("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								json j;
								ifff >> j;
								ifff.close();
								if (j["donated"] > 0) {
									Player::OnTextOverlay(peer, "Empty donation box first!");
									return;
								}
								remove(("save/donationboxes/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
							}
						}
						else if (world->items.at(x + (y * world->width)).foreground == 1240) {
							world->items.at(x + (y * world->width)).monitorname = "";
							world->items.at(x + (y * world->width)).monitoronline = false;
						} 
						else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::GATEWAY) {
							world->items.at(x + (y * world->width)).opened = false;
						}
						else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SHELF) {
							int shelf1 = 0;
							int shelf2 = 0;
							int shelf3 = 0;
							int shelf4 = 0;
							ifstream dshelf("save/dshelf/" + ((PlayerInfo*)(peer->data))->currentWorld + "/X" + std::to_string(squaresign) + ".txt");
							dshelf >> shelf1;
							dshelf >> shelf2;
							dshelf >> shelf3;
							dshelf >> shelf4;
							dshelf.close();
							bool isbreak = true;
							if (shelf1 != 0) {
								isbreak = false;
							}
							if (shelf2 != 0) {
								isbreak = false;
							}
							if (shelf3 != 0) {
								isbreak = false;
							}
							if (shelf4 != 0) {
								isbreak = false;
							}
							if (!isbreak) {
								Player::OnTalkBubble(peer, ((PlayerInfo*)(peer->data))->netID, "Empty the shelf before breaking it.", 0, true);
								return;
							}
						}
						else if (getItemDef(world->items.at(x + (y * world->width)).foreground).properties & Property_AutoPickup) {
							bool SuccessBreak = false;
							if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::VENDING) {
								if (world->items.at(x + (y * world->width)).vcount != 0 || world->items.at(x + (y * world->width)).vdraw != 0) {
									Player::OnTalkBubble(peer, pData->netID, "Empty the machine before breaking it!", 0, true);
									return;
								}
								world->items.at(x + (y * world->width)).vcount = 0;
								world->items.at(x + (y * world->width)).vprice = 0;
								world->items.at(x + (y * world->width)).vid = 0;
								world->items.at(x + (y * world->width)).vdraw = 0;
								world->items.at(x + (y * world->width)).opened = false;
								world->items.at(x + (y * world->width)).rm = false;
								auto success = true;
								SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success);
							} else if (world->items.at(x + (y * world->width)).foreground == 6286) {
								auto squaresign = x + (y * world->width);
								auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl1/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								if (isdbox) {
									ifstream ifff("save/storageboxlvl1/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
									json j;
									ifff >> j;
									ifff.close();
									if (j["instorage"] > 0) {
										Player::OnTextOverlay(peer, "Empty the box first!");
										return;
									}
									remove(("save/storageboxlvl1/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
								}
								bool success = true;
								SaveItemMoreTimes(6286, 1, peer, success);
							} else if (world->items.at(x + (y * world->width)).foreground == 6288) {
								auto squaresign = x + (y * world->width);
								auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl2/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								if (isdbox) {
									ifstream ifff("save/storageboxlvl2/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
									json j;
									ifff >> j;
									ifff.close();
									if (j["instorage"] > 0) {
										Player::OnTextOverlay(peer, "Empty the box first!");
										return;
									}
									remove(("save/storageboxlvl2/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
								}
								bool success = true;
								SaveItemMoreTimes(6288, 1, peer, success);
							} else if (world->items.at(x + (y * world->width)).foreground == 6290) {
								auto squaresign = x + (y * world->width);
								auto isdbox = std::experimental::filesystem::exists("save/storageboxlvl3/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								if (isdbox) {
									ifstream ifff("save/storageboxlvl3/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
									json j;
									ifff >> j;
									ifff.close();
									if (j["instorage"] > 0) {
										Player::OnTextOverlay(peer, "Empty the box first!");
										return;
									}
									remove(("save/storageboxlvl3/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
								}
								bool success = true;
								SaveItemMoreTimes(6290, 1, peer, success);
							} else if (world->items.at(x + (y * world->width)).foreground == 8878) {
								auto squaresign = x + (y * world->width);
								auto isdbox = std::experimental::filesystem::exists("save/safevault/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
								if (isdbox) {
									ifstream ifff("save/safevault/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json");
									json j;
									ifff >> j;
									ifff.close();
									if (j["insafe"] > 0) {
										Player::OnTextOverlay(peer, "Empty the safe first!");
										return;
									}
									remove(("save/safevault/_" + pData->currentWorld + "/X" + std::to_string(squaresign) + ".json").c_str());
								}
								bool success = true;
								SaveItemMoreTimes(8878, 1, peer, success);
							} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::LOCK) {
								if (world->items.at(x + (y * world->width)).foreground == 202 || world->items.at(x + (y * world->width)).foreground == 204 || world->items.at(x + (y * world->width)).foreground == 206 || world->items.at(x + (y * world->width)).foreground == 4994) {
									world->items.at(x + (y * world->width)).monitorname = "";
									world->items.at(x + (y * world->width)).opened = false;
									auto success = true;
									SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success);
									for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
										if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
										if (isHere(peer, net_peer)) {
											Player::PlayAudio(net_peer, "audio/metal_destroy.wav", 0);
										}
									}
								} else {
									if (world->category == "Guild") {
										Player::OnTalkBubble(peer, pData->netID, "Abandon your guild first before breaking the lock!", 0, true);
										return;
									}
									for (auto i = 0; i < world->width * world->height; i++) {
										if (getItemDef(world->items.at(i).foreground).properties & Property_Untradable && world->items.at(i).foreground != 0) {
											Player::OnTalkBubble(peer, pData->netID, "Take all untradeable blocks before breaking the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "", 0, true);
											return;
										}
										if (world->items.at(i).foreground == 1436)
										{
											Player::OnTalkBubble(peer, pData->netID, "`oRemove `1Security Camera `ofirst!", 0, true);
											return;
										}
									}
									if (pData->NickPrefix == "") updateworldremove(peer);
									auto success = true;
									SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success);
									for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
										if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
										if (isHere(peer, net_peer)) {
											Player::OnConsoleMessage(net_peer, "`5[`w" + world->name + " `ohas had its `$" + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " `oremoved!`5]");
											Player::PlayAudio(net_peer, "audio/metal_destroy.wav", 0);
										}
									} 
									if (world->owner != pData->rawName) {
										bool PlayerFound = false;
										for (ENetPeer* net_peer = server->peers; net_peer < &server->peers[server->peerCount]; ++net_peer) {
											if (net_peer->state != ENET_PEER_STATE_CONNECTED) continue;
											if (world->owner == static_cast<PlayerInfo*>(net_peer->data)->rawName) {
												PlayerFound = true;
												bool iscontainss = false;
												SearchInventoryItem(net_peer, 1424, 1, iscontainss);
												if (iscontainss) {
													RemoveInventoryItem(1424, 1, net_peer, true);
												}
												static_cast<PlayerInfo*>(net_peer->data)->worldsowned.erase(std::remove(static_cast<PlayerInfo*>(net_peer->data)->worldsowned.begin(), static_cast<PlayerInfo*>(net_peer->data)->worldsowned.end(), static_cast<PlayerInfo*>(net_peer->data)->currentWorld), static_cast<PlayerInfo*>(net_peer->data)->worldsowned.end());
												break;
											}
										} if (!PlayerFound) {
											try {
												ifstream read_player("save/players/_" + world->owner + ".json");
												if (!read_player.is_open()) {
													return;
												}	
												json j;
												read_player >> j;
												read_player.close();
												string WorldOwned = j["worldsowned"];
												vector<string> editworldsowned;
												stringstream ssfs(WorldOwned);
												while (ssfs.good()) {
													string substr;
													getline(ssfs, substr, ',');
													if (substr.size() == 0) continue;
													editworldsowned.push_back(substr);
												}
												editworldsowned.erase(std::remove(editworldsowned.begin(), editworldsowned.end(), pData->currentWorld), editworldsowned.end());
												string worldstring = "";
												for (int i = 0; i < editworldsowned.size(); i++) {
													worldstring += editworldsowned.at(i) + ",";
												}
												j["worldsowned"] = worldstring;
												ofstream write_player("save/players/_" + world->owner + ".json");
												write_player << j << std::endl;
												write_player.close();
											} catch (std::exception& e) {
												std::cout << e.what() << std::endl;
												return;
											}
										}
									} else {
										bool iscontainss = false;
										SearchInventoryItem(peer, 1424, 1, iscontainss);
										if (iscontainss) {
											RemoveInventoryItem(1424, 1, peer, true);
										}
										pData->worldsowned.erase(std::remove(pData->worldsowned.begin(), pData->worldsowned.end(), pData->currentWorld), pData->worldsowned.end());
									}
									world->rainbow = false;
									world->entrylevel = 1;
									world->owner = "";
									world->isPublic = false;
									world->accessed.clear();
									world->silence = false;
									world->publicBlock = -1;
									world->DisableDrop = false;
								}
								if (world->items.at(x + (y * world->width)).foreground == 4802) {
									for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
										if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
										if (isHere(peer, currentPeer)) {
											send_rainbow_shit_data(currentPeer, world->items.at(x + (y * world->width)).foreground, world->items.at(x + (y * world->width)).background, x, y, false, -1);
										}
									}
								}
							} else if (world->items.at(x + (y * world->width)).foreground == 6954) {
								if (world->items.at(x + (y * world->width)).mc != 0) {
									Player::OnTalkBubble(peer, pData->netID, "Empty the machine before breaking it!", 0, true);
									return;
								}
								world->items.at(x + (y * world->width)).mc = 0;
								world->items.at(x + (y * world->width)).mid = 0;
								world->items.at(x + (y * world->width)).vid = 0;
								world->items.at(x + (y * world->width)).rm = false;
								auto success = true;
								SaveItemMoreTimes(6954, 1, peer, success, "");
							} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::WEATHER || world->items.at(x + (y * world->width)).foreground == 3694 || world->items.at(x + (y * world->width)).foreground == 3832 || world->items.at(x + (y * world->width)).foreground == 5000) {
								auto success = true;
								SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success, "");
								if (world->weather != 0) {
									world->weather = 0;
									for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
										if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
										if (isHere(peer, currentPeer)) {
											Player::OnSetCurrentWeather(currentPeer, 0);
										}
									}
								}
								world->items.at(x + (y * world->width)).vid = 0; 
								world->items.at(x + (y * world->width)).vprice = 0; 
								world->items.at(x + (y * world->width)).vcount = 0; 
								world->items.at(x + (y * world->width)).intdata = 0;
								world->items.at(x + (y * world->width)).mc = 0;
								world->items.at(x + (y * world->width)).rm = false;
								world->items.at(x + (y * world->width)).opened = false;
								world->items.at(x + (y * world->width)).activated = false;
							} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).blockType == BlockTypes::SUCKER) {
								if (world->items.at(x + (y * world->width)).mid != 0 && world->items.at(x + (y * world->width)).mc != 0) {
									if (world->items.at(x + (y * world->width)).mc == 0) {
										world->items.at(x + (y * world->width)).mc = 0;
										world->items.at(x + (y * world->width)).mid = 0;
										world->items.at(x + (y * world->width)).rm = false;
										//world->items.at(x + (y * world->width)).foreground = 0;
										auto success = true;
										SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success, "");
									} else {
										Player::OnTalkBubble(peer, pData->netID, "Empty the " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " before breaking it!", 0, true);
										return;
									}
								} else {
									world->items.at(x + (y * world->width)).mc = 0;
									world->items.at(x + (y * world->width)).mid = 0;
									world->items.at(x + (y * world->width)).rm = false;
									//world->items.at(x + (y * world->width)).foreground = 0;
									auto success = true;
									SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success, "");
								}
							} else {
								auto success = true;
								SaveItemMoreTimes(world->items.at(x + (y * world->width)).foreground, 1, peer, success, "");
								world->items.at(x + (y * world->width)).mc = 0;
								world->items.at(x + (y * world->width)).mid = 0;
								world->items.at(x + (y * world->width)).rm = false;
								world->items.at(x + (y * world->width)).vcount = 0;
								world->items.at(x + (y * world->width)).vprice = 0;
								world->items.at(x + (y * world->width)).vid = 0;
								world->items.at(x + (y * world->width)).vdraw = 0;
								world->items.at(x + (y * world->width)).monitorname = "";
								world->items.at(x + (y * world->width)).monitoronline = false;
								world->items.at(x + (y * world->width)).opened = false;
								world->items.at(x + (y * world->width)).intdata = 0;
								//world->items.at(x + (y * world->width)).foreground = 0;
							}
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 3528) {
							if (world->items.at(x + (y * world->width)).intdata != 0) {
								world->items.at(x + (y * world->width)).intdata = 0;
							}
						} 
						else if (world->items.at(x + (y * world->width)).foreground == 2946) {
							if (world->items.at(x + (y * world->width)).intdata != 0) {
								auto success = true;
								SaveItemMoreTimes(world->items.at(x + (y * world->width)).intdata, 1, peer, success, pData->rawName + " from break " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + "");
								Player::OnTalkBubble(peer, pData->netID, "You picked up 1 " + getItemDef(world->items.at(x + (y * world->width)).intdata).name + ".", 0, true);
								world->items.at(x + (y * world->width)).intdata = 0;
							}
						} 
						if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity != 999 && world->items.at(x + (y * world->width)).foreground != 0) {
							SendTileData(world, peer, x, y, data.punchX, data.punchY, custom_drop);
							custom_drop = 0;
						}
						/*log mod dev*/
						if (!isWorldOwner(peer, world) && !isWorldAdmin(peer, world) && isDev(peer) && world->owner != "") {
							LogAccountActivity(pData->rawName, pData->rawName, "Break " + getItemDef(world->items.at(x + (y * world->width)).foreground).name + " (" + world->name + ")");
						}
						if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner && !isWorldAdmin(peer, world) && world->owner != "") /*Log moderator actions into security camera*/
						{
							auto islegitnow = false;
							for (auto i = 0; i < world->width * world->height; i++)
							{
								if (world->items[i].foreground == 1436)
								{
									islegitnow = true;
									break;
								}
							}
							if (islegitnow == true)
							{
								string toLogs = "";
								toLogs = static_cast<PlayerInfo*>(peer->data)->displayName + " `w(" + static_cast<PlayerInfo*>(peer->data)->rawName + "`w) `5Break `2" + getItemDef(world->items[x + (y * world->width)].foreground).name + "`5.";
								ofstream breaklogs("save/securitycam/logs/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".txt", ios::app);
								breaklogs << toLogs << endl;
								breaklogs.close();
							}
						}
						world->items.at(x + (y * world->width)).foreground = 0;
					} else {
						if (getItemDef(world->items.at(x + (y * world->width)).background).rarity != 999 && world->items.at(x + (y * world->width)).background != 0 && tile == 18) {
							if (getItemDef(world->items.at(x + (y * world->width)).background).properties & Property_Dropless) return;
							SendTileData(world, peer, x, y, data.punchX, data.punchY);
						}
						SendDropSeed(world, peer, x, y, world->items.at(x + (y * world->width)).background);
						world->items.at(x + (y * world->width)).background = 0;
						if (world->items.at(x + (y * world->width)).foreground == 1008 || world->items.at(x + (y * world->width)).foreground == 1796 || world->items.at(x + (y * world->width)).foreground == 242 || world->items.at(x + (y * world->width)).foreground == 9290 || world->items.at(x + (y * world->width)).foreground == 8470 || world->items.at(x + (y * world->width)).foreground == 8 || world->items.at(x + (y * world->width)).foreground == 9308) {
							world->items.at(x + (y * world->width)).foreground = 0;
						}
					}
					if (pData->quest_active && pData->lastquest == "honor" && pData->quest_step == 3 && pData->quest_progress < 5000) {
						pData->quest_progress++;
						if (pData->quest_progress >= 5000) {
							pData->quest_progress = 5000;
							Player::OnTalkBubble(peer, pData->netID, "`9Legendary Quest step complete! I'm off to see a Wizard!", 0, false);
						}
					}
				}
			} else {
				if (y < world->height) {
					world->items.at(x + (y * world->width)).breakTime = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
					world->items.at(x + (y * world->width)).breakLevel += 4;
				}
			}
			}
		} else {
			if (world->items.at(x + (y * world->width)).foreground != 0 && getItemDef(tile).blockType != BlockTypes::BACKGROUND && getItemDef(tile).blockType != BlockTypes::GROUND_BLOCK) return;
			if (!isWorldOwner(peer, world) && !isWorldAdmin(peer, world) && isDev(peer) && world->owner != "") {
				LogAccountActivity(pData->rawName, pData->rawName, "Place " + getItemDef(tile).name + " (" + world->name + ")");
			}
			for (auto i = 0; i < pData->inventory.items.size(); i++) {
				if (pData->inventory.items.at(i).itemID == tile) {
					if (static_cast<unsigned int>(pData->inventory.items.at(i).itemCount) > 1) {
						pData->inventory.items.at(i).itemCount--;
						pData->needsaveinventory = true;
					} else {
						pData->inventory.items.erase(pData->inventory.items.begin() + i);
						pData->needsaveinventory = true;
					}
					break;
				}
			}
			if (tile != 18 && tile != 32 && getItemDef(tile).blockType != BlockTypes::CONSUMABLE) {
				if (pData->PlacePotion || triple_place.find(pData->cloth_hand) != triple_place.end() || triple_place.find(pData->cloth_hair) != triple_place.end() || triple_place.find(pData->cloth_back) != triple_place.end() || triple_place.find(pData->cloth_necklace) != triple_place.end()) {
					SendPlacingEffect(peer, data.punchX, data.punchY, 229);
				} if (pData->cloth_hand == 9488) {
					SendPlacingEffect(peer, data.punchX, data.punchY, 150);
				} if (pData->cloth_back == 9152) {
					SendPlacingEffect(peer, data.punchX, data.punchY, 125);
				} 
			}
			if (getItemDef(tile).blockType == BlockTypes::BACKGROUND || getItemDef(tile).blockType == BlockTypes::GROUND_BLOCK) {
				world->items.at(x + (y * world->width)).background = tile;
			} else if (getItemDef(tile).blockType == BlockTypes::SEED) {
				world->items.at(x + (y * world->width)).foreground = tile;
			} else {
				world->items.at(x + (y * world->width)).foreground = tile;
			}
			world->items.at(x + (y * world->width)).breakLevel = 0;
		}
		if (!Explosion) {
			for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					auto raw = packPlayerMoving(&data);
					raw[2] = dicenr;
					raw[3] = dicenr;
					SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
				}
			} 
		} if (getItemDef(tile).properties & Property_MultiFacing) {
			if (pData->isRotatedLeft) {
				world->items.at(x + (y * world->width)).flipped = true;
				UpdateBlockState(peer, x, y, true, world);
			}
		} if (VendUpdate) {
			if (world->items.at(x + (y * world->width)).opened && world->items.at(x + (y * world->width)).vcount < world->items.at(x + (y * world->width)).vprice) {
				UpdateVend(peer, x, y, 0, false, world->items.at(x + (y * world->width)).vprice, world->items.at(x + (y * world->width)).foreground, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).opened);
			} else {
				UpdateVend(peer, x, y, world->items.at(x + (y * world->width)).vid, false, world->items.at(x + (y * world->width)).vprice, world->items.at(x + (y * world->width)).foreground, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).opened);
			}
		} if (isLock) {
			send_tile_data(peer, x, y, 0x10, world->items.at(x + (y * world->width)).foreground, world->items.at(x + (y * world->width)).background, lock_tile_datas(0x20, ((PlayerInfo*)(peer->data))->netID, 0, 0, false, 100));
		} if (isMannequin) {
			updateMannequin(peer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).sign, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, 0);
		} if (isSmallLock) {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					if (world->items.at(x + (y * world->width)).monitorname == pData->rawName) apply_lock_packet(world, currentPeer, x, y, world->items.at(x + (y * world->width)).foreground, pData->netID);
					else apply_lock_packet(world, currentPeer, x, y, world->items.at(x + (y * world->width)).foreground, -1);
				}
			}
		} if (isTree) {
			int growTimeSeed = getItemDef(world->items.at(x + (y * world->width)).foreground).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground).rarity;
			growTimeSeed += 30 * getItemDef(world->items.at(x + (y * world->width)).foreground).rarity;
			if (pData->quest_active && pData->lastquest == "honor" && pData->quest_step == 5 && pData->quest_progress < 50000) {
				pData->quest_progress += getItemDef(world->items.at(x + (y * world->width)).foreground).rarity;
				if (pData->quest_progress >= 50000) {
					pData->quest_progress = 50000;
					Player::OnTalkBubble(peer, pData->netID, "`9Legendary Quest step complete! I'm off to see a Wizard!", 0, false);
				}
			}
			world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + growTimeSeed);
			if (world->items.at(x + (y * world->width)).foreground == 5751) {
				world->items.at(x + (y * world->width)).fruitcount = 1;
			} else if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity == 999) {
				world->items.at(x + (y * world->width)).fruitcount = (rand() % 1) + 1;
			} else {
				world->items.at(x + (y * world->width)).fruitcount = (rand() % 5) + 1;
			}
			if (getItemDef(world->items.at(x + (y * world->width)).foreground - 1).blockType == BlockTypes::CLOTHING) world->items.at(x + (y * world->width)).fruitcount = (rand() % 2) + 1;
			if (world->items.at(x + (y * world->width)).foreground == 1791) world->items.at(x + (y * world->width)).fruitcount = 1;
			int chanceofbuff = 1;
			if (pData->level >= 8) chanceofbuff = 1;
			if (pData->level >= 11) chanceofbuff = 2;
			if (pData->level >= 8 && rand() % 100 <= chanceofbuff) {
				Player::OnConsoleMessage(peer, "Flawless bonus reduced 1 hour grow time");
				int NewGrowTime = 0;
				int InternalGrowTime = 0;
				NewGrowTime = calcBanDuration(world->items.at(x + (y * world->width)).growtime) - 3600;
				if (NewGrowTime < 0) NewGrowTime = 0;
				world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + NewGrowTime);	
				int growTimeSeed = getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
				growTimeSeed += 30 * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
				InternalGrowTime = growTimeSeed - calcBanDuration(world->items.at(x + (y * world->width)).growtime);
				UpdateTreeVisuals(peer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).fruitcount, InternalGrowTime, true, 0);
			}
			spray_tree(peer, world, x, y, 18, true);
		} if (isScience) {
			world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + getItemDef(world->items.at(x + (y * world->width)).foreground).growTime);
		} if (isHeartMonitor) {
			world->items.at(x + (y * world->width)).monitorname = pData->displayName;
			world->items.at(x + (y * world->width)).monitoronline = true;
			sendHMonitor(peer, x, y, pData->displayName, true, world->items.at(x + (y * world->width)).background);
		} if (isgateway) {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					if (isDev(currentPeer) || isWorldOwner(currentPeer, world) || isWorldAdmin(currentPeer, world)) {
						update_entrance(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, true, world->items.at(x + (y * world->width)).background);
					}
					else {
						update_entrance(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).opened, world->items.at(x + (y * world->width)).background);
					}
				}
			}
		} 
		if (isvip) {
			for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
				if (isHere(peer, currentPeer)) {
					if (isDev(currentPeer) || isWorldOwner(currentPeer, world) || isWorldAdmin(currentPeer, world) || isAdminVipEntrance(peer, world, x, y)) {
						update_entrance(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, true, world->items.at(x + (y * world->width)).background);
					}
					else
					{
						update_entrance(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).opened, world->items.at(x + (y * world->width)).background);
					}
				}
			}
		}
		if (isDoor) {
			for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
				if (isHere(peer, currentPeer)) {
					if (isWorldOwner(currentPeer, world) || isWorldAdmin(currentPeer, world) || isDev(currentPeer)) {
						updateDoor(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, getItemDef(world->items.at(x + (y * world->width)).foreground).name, false, false);
					}
					else
					{
						updateDoor(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, getItemDef(world->items.at(x + (y * world->width)).foreground).name, true, false);
					}
				}
			}
		} if (isMagplant) {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					send_item_sucker(currentPeer, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).mid, -1, true, true, world->items.at(x + (y * world->width)).background);
				}
			}
		}
	} 
	catch(const std::out_of_range& e) {
		std::cout << e.what() << std::endl;
	} 
}


inline void SendSacrifice(WorldInfo* world, ENetPeer* peer, int itemid, int count)
{



	int pradinis = itemid;
	
	int Rarity = getItemDef(itemid).rarity;
	if (getItemDef(itemid).rarity >= 10)
	{
		Rarity = getItemDef(itemid).rarity * count;
	}

	if (itemid == 242) itemid = 1212;
	else if (itemid == 1190) itemid = 1214;
	else if (itemid == 882) itemid = 1232;
	else if (itemid == 592 || itemid == 1018) itemid = 1178;
	else if (itemid == 362) itemid = 1234;
	else if (itemid == 910) itemid = 1250;
	else if (itemid == 274 || itemid == 276) itemid = 1956;
	else if (itemid == 1474) itemid = 1990;
	else if (itemid == 1506) itemid = 1968;
	else if (itemid == 1746) itemid = 1960;
	else if (itemid == 1252) itemid = 1948;
	else if (itemid == 1830) itemid = 1966;
	else if (itemid == 2722) itemid = 3114;
	else if (itemid == 2984) itemid = 3118;
	else if (itemid == 3040) itemid = 3100;
	else if (itemid == 2390) itemid = 3122;
	else if (itemid == 1934) itemid = 3124;
	else if (itemid == 1162) itemid = 3126;
	else if (itemid == 604 || itemid == 2636) itemid = 3108;
	else if (itemid == 3020) itemid = 3098;
	else if (itemid == 914 || itemid == 916 || itemid == 918 || itemid == 920 || itemid == 924) itemid = 1962;
	else if (itemid == 900 || itemid == 1378 || itemid == 1576 || itemid == 7136 || itemid == 7754 || itemid == 7752 || itemid == 7758 || itemid == 7760) itemid = 2500;
	else if (itemid == 1460) itemid = 1970;
	else if (itemid == 3556 && rand() % 100 <= 50) itemid = 4186;
	else if (itemid == 3556) itemid = 4188;
	else if (itemid == 2914 || itemid == 3012 || itemid == 3014) itemid = 4246;
	else if (itemid == 3016 || itemid == 3018) itemid = 4248;
	else if (itemid == 414 || itemid == 416 || itemid == 418 || itemid == 420 || itemid == 422 || itemid == 424 || itemid == 426 || itemid == 4634 || itemid == 4636 || itemid == 4638 || itemid == 4640 || itemid == 4642) itemid = 4192;
	else if (itemid == 1114) itemid = 4156;
	else if (itemid == 366) itemid = 4136;
	else if (itemid == 1950) itemid = 4152;
	else if (itemid == 2386) itemid = 4166;
	else if (itemid == 762) itemid = 4190;
	else if (itemid == 2860 || itemid == 2268) itemid = 4172;
	else if (itemid == 2972) itemid = 4182;
	else if (itemid == 3294) itemid = 4144;
	else if (itemid == 3296) itemid = 4146;
	else if (itemid == 3298) itemid = 4148;
	else if (itemid == 3290) itemid = 4140;
	else if (itemid == 3288) itemid = 4138;
	else if (itemid == 3292) itemid = 4142;
	else if (itemid == 1198) itemid = 5256;
	else if (itemid == 4960) itemid = 5208;
	else if (itemid == 1242) itemid = 5216;
	else if (itemid == 1244) itemid = 5218;
	else if (itemid == 1248) itemid = 5220;
	else if (itemid == 1246) itemid = 5214;
	else if (itemid == 5018) itemid = 5210;
	else if (itemid == 1408) itemid = 5254;
	else if (itemid == 4334) itemid = 5250;
	else if (itemid == 4338) itemid = 5252;
	else if (itemid == 3792) itemid = 5244;
	else if (itemid == 1294) itemid = 5236;
	else if (itemid == 6144) itemid = 7104;
	else if (itemid == 4732) itemid = 7124;
	else if (itemid == 4326) itemid = 7122;
	else if (itemid == 6300) itemid = 7102;
	else if (itemid == 2868) itemid = 7100;
	else if (itemid == 6798) itemid = 7126;
	else if (itemid == 6160) itemid = 7104;
	else if (itemid == 6176) itemid = 7124;
	else if (itemid == 5246) itemid = 7122;
	else if (itemid == 5246) itemid = 7102;
	else if (itemid == 5246) itemid = 7100;
	else if (itemid == 7998) itemid = 9048;
	else if (itemid == 6196) itemid = 9056;
	else if (itemid == 2392) itemid = 9114;
	else if (itemid == 8018) itemid = 9034;
	else if (itemid == 8468) itemid = 10232;

	else if (itemid == 4360) itemid = 10194;
	else if (itemid == 9364) itemid = 10206;
	else if (itemid == 9322) itemid = 10184;

	else if (itemid == 3818) itemid = 10192;
	else if (itemid == 3794) itemid = 10190;
	else if (itemid == 7696) itemid = 10186;

	else if (itemid == 10212) itemid = 10212;

	else if (itemid == 10328)
	{
		int itemuMas[154] = { 10232, 10194, 10206, 10184, 10192, 10190, 10186, 10212, 1212, 1190, 1206, 1166, 1964, 1976, 1998, 1946, 2502, 1958, 1952, 2030, 3104, 3112, 3120, 3092, 3094, 3096, 4184, 4178, 4174, 4180, 4170, 4168, 4150, 1180, 1224, 5226, 5228, 5230, 5212, 5246, 5242, 5234, 7134, 7118, 7132, 7120, 7098, 9018, 9038, 9026, 9066, 9058, 9044, 9024, 9032, 9036, 9028, 9030, 9110, 9112, 10386, 10326, 10324, 10322, 10328, 10316, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 1236, 1182, 1184, 1186, 1188, 1170, 1212, 1214, 1232, 1178, 1234, 1250, 1956, 1990, 1968, 1960, 1948, 1966, 3114, 3118, 3100, 3122, 3124, 3126, 3108, 3098, 1962, 2500, 1970, 4186, 4188, 4246, 4248, 4192, 4156, 4136, 4152, 4166, 4190, 4172, 4182, 4144, 4146, 4148, 4140, 4138, 4142, 5256, 5208, 5216, 5218, 5220, 5214, 5210, 5254, 5250, 5252, 5244, 5236, 7104, 7124, 7122, 7102, 7100, 7126, 7104, 7124, 7122, 7102, 7100, 9048, 9056, 9114, 9034 };
		auto randIndex = rand() % 154;
		itemid = itemuMas[randIndex];
		playerRespawn(world, peer, false);
		Player::OnConsoleMessage(peer, "`2" + getItemDef(pradinis).name + " `obu odul `4Growganoth `otarafindan verildi!");
		Player::OnConsoleMessage(peer, "`4Growganoth senden cok ozel bir item aldi ve sana kendi koleksiyonundan ozel bir item verdi, servisini sonlandirdi!");
		Player::OnConsoleMessage(peer, "`oBir `2" + getItemDef(itemid).name + " `oozel item sana verildi!");
		if (getItemDef(itemid).name.find("Wings") != string::npos || getItemDef(itemid).name.find("Cape") != string::npos || getItemDef(itemid).name.find("Cloak") != string::npos)
		{
			ENetPeer* currentPeer;
			for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				Player::OnConsoleMessage(currentPeer, "`4Growganoth'dan `w" + static_cast<PlayerInfo*>(peer->data)->displayName + " `oodullendirilerek bir `5Degerli " + getItemDef(itemid).name + "`okazandi.");
			}
		}
		auto success = true;
		SaveItemMoreTimes(itemid, 1, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " growganoth sana gonderdi");
		if (count >= 2)
		{
			auto success = true;
			SaveItemMoreTimes(10328, count - 1, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " growganoth sana gonderdi");
		}
		return;
	}

	else if (getItemDef(itemid).blockType == BlockTypes::WEATHER && itemid != 932) itemid = 1210;

	else if (Rarity < 10)
	{
		int itemuMas[2] = { 1208, 5256 };
		auto randIndex = rand() % 2;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 15)
	{
		int itemuMas[5] = { 1222, 1198, 1992, 5256, 1208 };
		auto randIndex = rand() % 5;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 20)
	{
		int itemuMas[7] = { 1250, 1992, 1982, 5256, 1198, 1208, 1222 };
		auto randIndex = rand() % 7;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 25)
	{
		int itemuMas[9] = { 1220, 1992, 1982, 5256, 1198, 1208, 1222, 1250, 10198 };
		auto randIndex = rand() % 9;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 30)
	{
		int itemuMas[11] = { 1202, 1992, 1982, 5240, 5256, 1198, 1208, 1222, 1250, 1220, 10198 };
		auto randIndex = rand() % 11;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 35)
	{
		int itemuMas[17] = { 1238, 1992, 1982, 4160, 4162, 5240, 5238, 5256, 7116, 1198, 1208, 1222, 1250, 1220, 1202, 10198, 10196 };
		auto randIndex = rand() % 17;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 40)
	{
		int itemuMas[18] = { 1168, 1992, 1982, 4160, 4162, 5240, 5238, 5256, 7116, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 10198, 10196 };
		auto randIndex = rand() % 18;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 45)
	{
		int itemuMas[21] = { 1172, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 10198, 10196 };
		auto randIndex = rand() % 21;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 50)
	{
		int itemuMas[22] = { 1230, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 10198, 10196 };
		auto randIndex = rand() % 22;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 55)
	{
		int itemuMas[25] = { 1194, 1192, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 10198, 10196, 10202 };
		auto randIndex = rand() % 25;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 60)
	{
		int itemuMas[27] = { 1226, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 7108, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 10198, 10196, 10202 };
		auto randIndex = rand() % 27;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 65)
	{
		int itemuMas[28] = { 1196, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 7108, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 10198, 10196, 10202 };
		auto randIndex = rand() % 28;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 70)
	{
		int itemuMas[29] = { 1236, 1992, 1982, 3116, 4160, 4162, 4164, 5240, 5238, 5256, 7116, 7108, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 10198, 10196, 10202 };
		auto randIndex = rand() % 29;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 80)
	{
		int itemuMas[44] = { 1182, 1184, 1186, 1188, 1992, 1982, 1994, 1972, 1980, 1988, 3116, 3102, 4160, 4162, 4164, 4154, 5224, 5222, 5232, 5240, 5238, 5256, 7116, 7108, 7110, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 1236, 10198, 10196, 10202, 10204 };
		auto randIndex = rand() % 44;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity < 90)
	{
		int itemuMas[48] = { 1170, 1992, 1982, 1994, 1972, 1980, 1988, 1984, 3116, 3102, 4160, 4162, 4164, 4154, 4158, 5224, 5222, 5232, 5240, 5238, 5256, 7116, 7108, 7110, 7128, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 1236, 1182, 1184, 1186, 1188, 10198, 10196, 10202, 10204 };
		auto randIndex = rand() % 48;
		itemid = itemuMas[randIndex];
		count = 1;
	}
	else if (Rarity >= 90 && Rarity != 999)
	{
		int itemuMas[56] = { 1216, 1218, 1992, 1982, 1994, 1972, 1980, 1988, 1984, 3116, 3102, 3106, 3110, 4160, 4162, 4164, 4154, 4158, 5224, 5222, 5232, 5240, 5248, 5238, 5256, 7116, 7108, 7110, 7128, 7112, 7114, 7130, 1198, 1208, 1222, 1250, 1220, 1202, 1238, 1168, 1172, 1230, 1194, 1192, 1226, 1196, 1236, 1182, 1184, 1186, 1188, 1170, 10198, 10196, 10202, 10204 };
		auto randIndex = rand() % 56;
		itemid = itemuMas[randIndex];
		count = 1;
	}

	if (pradinis == itemid)
	{
		playerRespawn(world, peer, false);
		Player::OnConsoleMessage(peer, "`4Growganoth bu teklifinizi begenmedi ve item yerine sizi yedi!");
		Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`4Growganoth bu teklifinizi begenmedi ve item yerine sizi yedi!", 0, true);
		auto success = true;
		SaveItemMoreTimes(pradinis, count, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " growganoth sana gonderdi");
		return;
	}

	playerRespawn(world, peer, false);
	Player::OnConsoleMessage(peer, "`2" + getItemDef(pradinis).name + " `ogrowganoth tarafindan yendi ve!");
	Player::OnConsoleMessage(peer, "`oA `2" + getItemDef(itemid).name + " `oodul olarak sana verildi!");
	auto success = true;
	SaveItemMoreTimes(itemid, count, peer, success, static_cast<PlayerInfo*>(peer->data)->rawName + " growganoth sana gonderdi");

}

inline void SendChat(ENetPeer* peer, const int netID, string message, WorldInfo* world, string cch) {		
	if (GlobalMaintenance) return;
	if (message.find("player_chat=") != string::npos) {
		return;
	}
	try {
		if (message.length() >= 120 || message.length() == 0 || message == " " || !static_cast<PlayerInfo*>(peer->data)->isIn || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || 1 > (message.size() - countSpaces(message))) return;
		if (static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT") return;			
		removeExtraSpaces(message);
		string str = message;
		if (str.rfind("/", 0) == 0) {
			if (!static_cast<PlayerInfo*>(peer->data)->haveGrowId) {
				Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "Lutfen ilk once hesabinizi olusturun!", 0, true);
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->trade) end_trade(peer);
			if (str != "/togglemod" && !has_permission(static_cast<PlayerInfo*>(peer->data)->adminLevel, str, static_cast<PlayerInfo*>(peer->data)->Subscriber)) {
				sendWrongCmd(peer);
				return;
			}
		}
		else {
			if (std::string::npos != str.find("allah") || std::string::npos != str.find("orospu") || std::string::npos != str.find("siktir") || std::string::npos != str.find("yarram") || std::string::npos != str.find("yarrak") || std::string::npos != str.find("sikiyim") || std::string::npos != str.find("sokayim") || std::string::npos != str.find("anani") || std::string::npos != str.find("anan") || std::string::npos != str.find("sg") || std::string::npos != str.find("OROSPU") || std::string::npos != str.find("yarr") || std::string::npos != str.find("amk") || std::string::npos != str.find("pic") || std::string::npos != str.find("sik") || std::string::npos != str.find("sex") || std::string::npos != str.find("meme") || std::string::npos != str.find("OROSPU") || std::string::npos != str.find("SIKTIR") || std::string::npos != str.find("amc") || std::string::npos != str.find("ata") || std::string::npos != str.find("anan") || std::string::npos != str.find("crows")) {
			//send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5sistem yasaklanmis kelime yakaladi: `1" + str + "\n");
			}
		}
		if (static_cast<PlayerInfo*>(peer->data)->messagecooldown)
		{

			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (!isDev(peer) && !isMod(currentPeer)) {
						Player::OnConsoleMessage(peer, "`6>>`4Spam Decected! `6Please wait a bit before typing anything else. Please note, any form of bot/macro/auto-paste will get all your accounts banned so don't do it!");
						return;
					}
				
			}
			
		}

		if (static_cast<PlayerInfo*>(peer->data)->isantibotquest)
		{
			Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wSen bir Insan Misin?``|left|206|\nadd_spacer|small|\nadd_textbox|`oInsan oldugunu kanitlamak icin sorulan soruyu dogru bir sekilde cevapla.|\nadd_textbox|" + to_string(static_cast<PlayerInfo*>(peer->data)->antibotnumber1) + " + " + to_string(static_cast<PlayerInfo*>(peer->data)->antibotnumber2) + "|\nadd_text_input|inputantibot|`oAnswer:||26|\nend_dialog|antibotsubmit||`wOnayla|\n");
			return;
		}
		if (str.at(0) == '/')
		{
			static_cast<PlayerInfo*>(peer->data)->commandsused = static_cast<PlayerInfo*>(peer->data)->commandsused + 1;
			if (static_cast<PlayerInfo*>(peer->data)->commandsused >= 8)
			{

				if (!isMod(peer)) {

					static_cast<PlayerInfo*>(peer->data)->messagecooldown = true;
					static_cast<PlayerInfo*>(peer->data)->messagecooldowntime = GetCurrentTimeInternalSeconds() + 25;
					Player::OnConsoleMessage(peer, "`6>>`4Spam Decected! `6Please wait a bit before typing anything else. Please note, any form of bot/macro/auto-paste will get all your accounts banned so don't do it!");
					return;
				}
			}
		}
		if (str == "/ghost") {
			if ((world->width == 90 && world->height == 60)|| world->height == 110) {
				Player::OnConsoleMessage(peer, "Bu komutu burada kullanamazsin.");
				return;
			}
			if (world->name == "GAME1") {
				Player::OnConsoleMessage(peer, "Bu komutu burada kullanamazsin.");
				return;
			}
			if (world->name == "BASLA") {
				Player::OnConsoleMessage(peer, "Bu komutu burada kullanamazsin.");
				return;
			}
			if (world->name == "CSN") {
				Player::OnConsoleMessage(peer, "Kumar oynarken hayalet olmak mi?!?");
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->isrobbing)
			{
				Player::OnConsoleMessage(peer, "Bu komutu hirsizlik yaparken kullanamazsin.");
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->job != "")
			{
				Player::OnConsoleMessage(peer, "Bu komutu is yaparken kullanamazsin.");
				return;
			}
			SendGhost(peer);
		}
		else if(str == "/removewantedstars")
		{
			static_cast<PlayerInfo*>(peer->data)->wantedstars = 0;
			Player::OnTextOverlay(peer, "Wanted stars were removed.");
		}
		else if(str == "/removejobcooldown")
		{
			static_cast<PlayerInfo*>(peer->data)->joinjobscooldown = 0;
			Player::OnTextOverlay(peer, "Cooldown was removed.");
		}
		else if(str == "/escapefromjail")
		{
			if(static_cast<PlayerInfo*>(peer->data)->updateJailedTime != 0 && static_cast<PlayerInfo*>(peer->data)->jailedtime > 0)
			{
				static_cast<PlayerInfo*>(peer->data)->isjailed = false;
				static_cast<PlayerInfo*>(peer->data)->updateJailedTime = 0;
				Player::OnTextOverlay(peer, "Hapisten ciktiniz. Bir daha buraya gelmek istemiyorsaniz suca bulasmayin.");
				Player::OnConsoleMessage(peer, "Hapisten ciktiniz. Bir daha buraya gelmek istemiyorsaniz suca bulasmayin.");
				handle_world(peer, "BASLA");
			}
			else
			{
				Player::OnTextOverlay(peer, "Hapiste degilsin.");
			}
		}
		else if (str.substr(0, 16) == "/clearinventory ") {
			string growid = PlayerDB::getProperName(str.substr(16, cch.length() - 16 - 1));
			bool found = false;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == growid)
				{
					try {
						static_cast<PlayerInfo*>(currentPeer->data)->inventory.items.clear();
						sendClothes(currentPeer);

						bool success = false;
						SaveItemMoreTimes(18, 1, currentPeer, success);
						SaveItemMoreTimes(32, 1, currentPeer, success);
						SaveItemMoreTimes(6336, 1, currentPeer, success);

						Player::OnTextOverlay(peer, "`oInventory `1cleared `ofor user `2" + growid);
					}
					catch (const std::out_of_range& e) {
						std::cout << e.what() << std::endl;
					}
					found = true;
					break;
				}
			}
			if (!found)
			{
				Player::OnTextOverlay(peer, "`2" + growid + " `1is offline");
				return;
			}
		}
		else if (str.substr(0, 4) == "/gc ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->guild == "")
			{
				sendWrongCmd(peer);
				return;
			}
			string textinfo = str.substr(4, cch.length() - 4 - 1);
			if (textinfo.size() < 1)
			{
				sendWrongCmd(peer);
				return;
			}
			GuildInfo* guild = guildDB.get_pointer(static_cast<PlayerInfo*>(peer->data)->guild);
			if (guild == NULL)
			{
				Player::OnConsoleMessage(peer, "`4An error occurred while getting guilds information! `1(#1)`4.");
				return;
			}
			int onlinecount = 0;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->guild == static_cast<PlayerInfo*>(peer->data)->guild)
				{
					if (Getguildcheckbox_notifications(guild, static_cast<PlayerInfo*>(currentPeer->data)->rawName))
					{
						if(Getguildcheckbox_public(guild, static_cast<PlayerInfo*>(peer->data)->rawName))
							Player::OnConsoleMessage(currentPeer, "`2>> from (``"+ static_cast<PlayerInfo*>(peer->data)->displayName+" "+ GuildGetRankName(guild, static_cast<PlayerInfo*>(peer->data)->rawName, false)+"`````2) in [```$"+ static_cast<PlayerInfo*>(peer->data)->currentWorld+"```2] > ```$"+ textinfo +"``");
						else
							Player::OnConsoleMessage(currentPeer, "`2>> from (``"+ static_cast<PlayerInfo*>(peer->data)->displayName +" "+ GuildGetRankName(guild, static_cast<PlayerInfo*>(peer->data)->rawName, false) +"`````2) > ```$"+ textinfo +"``");
						
						Player::PlayAudio(currentPeer, "audio/friend_beep.wav", 0);
						onlinecount++;
					}
				}
			}

			Player::OnConsoleMessage(peer, "`2>> You guildcasted to `w"+to_string(onlinecount)+"`` people online.``");
		}
		else if (str.substr(0, 3) == "/a ")
		{
		if (!ismathevent)
		{
			Player::OnTextOverlay(peer, "`oMatematik Eventi `4daha baslamadi!");
			return;
		}
		int answer = atoi(str.substr(3, cch.length() - 3 - 1).c_str());

		if (answer != matheventanswer)
		{
			Player::OnTextOverlay(peer, "`4Yanlis cevap.");
			return;
		}

		ismathevent = false;

		std::ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
		std::string content((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
		int gembux = atoi(content.c_str());
		int fingembux = gembux + matheventprize;
		ofstream myfile;
		myfile.open("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
		myfile << fingembux;
		myfile.close();
		int gemcalc = gembux + matheventprize;
		Player::OnSetBux(peer, gemcalc, 0);

		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
			Player::OnConsoleMessage(currentPeer, "`5Matematik Eventi `1kazanani `o" + static_cast<PlayerInfo*>(peer->data)->displayName + "`o. `2Tebrikler `5" + to_string(matheventprize) + " (gems) kazandin.");
			Player::PlayAudio(currentPeer, "pinata_lasso.wav", 0);
		}
		}
		else if (str == "/clear")
		{
			if(world->name == "START" || world->name == "SEHIRS" || world->name == "BASLA" || world->name == "GAME1" || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
			{
				Player::OnTextOverlay(peer, "Orospu evladi burada niye deniyon.");
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->isclearcommandcooldown)
			{
				Player::OnTextOverlay(peer, "`o30 Dakika bekle bakam.");
				return;
			}
			LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "/clear " + world->name);
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5kullandi `4/clear `2" + world->name);
			static_cast<PlayerInfo*>(peer->data)->clearcommandcooldown = GetCurrentTimeInternalSeconds() + 1800;
			static_cast<PlayerInfo*>(peer->data)->isclearcommandcooldown = true;
			for (auto i = 0; i < world->width * world->height; i++)
			{
				if (world->items[i].foreground == 6 || world->items[i].foreground == 8 || world->items[i].foreground == 242 || world->items[i].foreground == 2408 || world->items[i].foreground == 1796 || world->items[i].foreground == 4428 || world->items[i].foreground == 7188 || world->items[i].foreground == 8470 || world->items[i].foreground == 9290 || world->items[i].foreground == 9308 || world->items[i].foreground == 9504 || world->items[i].foreground == 2950 || world->items[i].foreground == 4802 || world->items[i].foreground == 5260 || world->items[i].foreground == 5814 || world->items[i].foreground == 5980 || world->items[i].foreground == 9640) continue;
				if (world->items[i].foreground != 0 || world->items[i].background != 0)
				{
					world->items[i].foreground = 0;
					world->items[i].background = 0;
					PlayerMoving data3;
					data3.packetType = 0x3;
					data3.characterState = 0x0;
					data3.x = i % world->width;
					data3.y = i / world->height;
					data3.punchX = i % world->width;
					data3.punchY = i / world->width;
					data3.XSpeed = 0;
					data3.YSpeed = 0;
					data3.netID = -1;
					data3.plantingTree = 0;
					for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6)
					{
						if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
						if (isHere(peer, currentPeer6))
						{
							static_cast<PlayerInfo*>(currentPeer6->data)->lastx = 0; //antihack last pos
							static_cast<PlayerInfo*>(currentPeer6->data)->lasty = 0; // antihack last pos
							static_cast<PlayerInfo*>(currentPeer6->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 3;
							static_cast<PlayerInfo*>(currentPeer6->data)->disableanticheat = true;

							auto raw = packPlayerMoving(&data3);
							raw[2] = 0;
							raw[3] = 0;
							SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
						}
					}
				}
			}
		}
		else if(str == "/enablelogs")
		{
			if(!enabledGlobalLogs)
			{
				enabledGlobalLogs = true;
				Player::OnTextOverlay(peer, "Logs were enabled. If you want to turn off them, use this command again");
			}
			else
			{
				enabledGlobalLogs = false;
				Player::OnTextOverlay(peer, "Logs were disabled. If you want to turn on them, use this command again");
			}
		}
		else if(str == "/hitman")
		{
			if(static_cast<PlayerInfo*>(peer->data)->job != "hitman")
			{
				Player::OnTextOverlay(peer, "`oYou need to get `1\"The Hitman\" `ojob");
				return;
			}
			GTDialog hitman;
			hitman.addLabelWithIcon("List Of Orders", 4488, LABEL_BIG);
			hitman.addSpacer(SPACER_SMALL);
			int counter = 0;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if(static_cast<PlayerInfo*>(currentPeer->data)->hitmanbet > 0)
				{
					hitman.addSmallText(static_cast<PlayerInfo*>(currentPeer->data)->displayName+"`o, Reward for killing: `1"+to_string(static_cast<PlayerInfo*>(currentPeer->data)->hitmanbet)+" `ogems.");
					hitman.addSpacer(SPACER_SMALL);
					counter++;
				}
			}
			
			if(counter > 0)
			{
				hitman.addSpacer(SPACER_SMALL);
				hitman.addSmallText("Find the player from that List and kill him with `1"+itemDefs[7586].name);
			}
			else
			{
				hitman.addSmallText("No one is currently on the List!");
			}
			hitman.addSpacer(SPACER_SMALL);
			hitman.addQuickExit();
			hitman.endDialog("Close", "", "EXIT");
			Player::OnDialogRequest(peer, hitman.finishDialog());	
		}
		else if(str == "/wantedlist")
		{
			if(static_cast<PlayerInfo*>(peer->data)->job != "police")
			{
				Player::OnTextOverlay(peer, "`oYou need to get `1\"A Police Officer\" `ojob");
				return;
			}

			GTDialog wantedlist;
			wantedlist.addLabelWithIcon("Wanted List", 948, LABEL_BIG);
			wantedlist.addSpacer(SPACER_SMALL);
			int counter = 0;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars > 0)
				{
					wantedlist.addSmallText("`1"+static_cast<PlayerInfo*>(currentPeer->data)->displayName+", `oWanted stars: `1"+to_string(static_cast<PlayerInfo*>(currentPeer->data)->wantedstars));
					wantedlist.addSpacer(SPACER_SMALL);
					counter++;
				}
			}
			if(counter > 0)
			{
				wantedlist.addSpacer(SPACER_SMALL);
				wantedlist.addSmallText("Find the player from that Wanted List and cuff him with Handscuff. You will get a reward!");
			}
			else
			{
				wantedlist.addSmallText("No one is currently on the wanted list!");
			}
			wantedlist.addSpacer(SPACER_SMALL);
			wantedlist.addQuickExit();
			wantedlist.endDialog("Close", "", "EXIT");
			Player::OnDialogRequest(peer, wantedlist.finishDialog());				
		}
		else if (str == "/freezeall") {
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5used`1: `9/freezeall");
			int howmuch = 0;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer) && static_cast<PlayerInfo*>(currentPeer->data)->rawName != static_cast<PlayerInfo*>(peer->data)->rawName)
					{
						try {
							if (!static_cast<PlayerInfo*>(currentPeer->data)->frozen)
							{
								GamePacket p2 = packetEnd(appendIntx(appendString(createPacket(), "OnSetFreezeState"), 1));
								memcpy(p2.data + 8, &(((PlayerInfo*)(currentPeer->data))->netID), 4);
								ENetPacket* packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
								enet_peer_send(currentPeer, 0, packet2);
								delete p2.data;
								static_cast<PlayerInfo*>(currentPeer->data)->skinColor = -125000;
								sendClothes(currentPeer);

								Player::OnTextOverlay(currentPeer, "`oYou have been `1frozen `oby a mod.");
								Player::OnConsoleMessage(currentPeer, "`oThere are so icy right i am so cold now!? (`oFrozen mod added! 30 Seconds Left`o)");
								static_cast<PlayerInfo*>(currentPeer->data)->frozen = true;
								static_cast<PlayerInfo*>(currentPeer->data)->freezetime = GetCurrentTimeInternalSeconds() + 30;
								Player::PlayAudio(currentPeer, "audio/freeze.wav", 0);
							}
							else
							{
								GamePacket p2 = packetEnd(appendIntx(appendString(createPacket(), "OnSetFreezeState"), 0));
								memcpy(p2.data + 8, &(((PlayerInfo*)(currentPeer->data))->netID), 4);
								ENetPacket* packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
								enet_peer_send(currentPeer, 0, packet2);
								delete p2.data;
								static_cast<PlayerInfo*>(currentPeer->data)->skinColor = -2104114177;
								sendClothes(currentPeer);

								Player::OnTextOverlay(currentPeer, "`oYou have been `1unfrozen `oby a mod.");
								Player::OnConsoleMessage(currentPeer, "`oThere are so icy right i am so cold now!? (`oFrozen mod added! 30 Seconds Left`o)");
								static_cast<PlayerInfo*>(currentPeer->data)->frozen = false;
								static_cast<PlayerInfo*>(currentPeer->data)->freezetime = 0;
							}
							howmuch++;
						}
						catch (const std::out_of_range& e) {
							std::cout << e.what() << std::endl;
						}
					}
			}
			Player::OnTextOverlay(peer, "`4" + to_string(howmuch) + " `oplayers were `1frozen`o.");
		}
		else if (str.substr(0, 12) == "/setweather ")
		{
			string ids = PlayerDB::getProperName(str.substr(12, cch.length() - 12 - 1));
			int id = atoi(ids.c_str());
			world->weather = id;
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (isHere(peer, currentPeer))
				{
					GamePacket p2 = packetEnd(appendInt(appendString(createPacket(), "OnSetCurrentWeather"), world->weather));
					ENetPacket* packet2 = enet_packet_create(p2.data,
						p2.len,
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet2);
					delete p2.data;
					continue;
				}
			}
		}
		else if (str.substr(0, 8) == "/freeze ") {
			string growid = PlayerDB::getProperName(str.substr(8, cch.length() - 8 - 1));
			bool found = false;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == growid)
				{
					try {
						if (!static_cast<PlayerInfo*>(currentPeer->data)->frozen)
						{
							GamePacket p2 = packetEnd(appendIntx(appendString(createPacket(), "OnSetFreezeState"), 1));
							memcpy(p2.data + 8, &(((PlayerInfo*)(currentPeer->data))->netID), 4);
							ENetPacket* packet2 = enet_packet_create(p2.data,p2.len, ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(currentPeer, 0, packet2);
							delete p2.data;
							static_cast<PlayerInfo*>(currentPeer->data)->skinColor = -119500;
							sendClothes(currentPeer);

							Player::OnTextOverlay(peer, "`2" + growid + " `1is frozen.");
							Player::OnTextOverlay(currentPeer, "`oYou have been `1frozen `oby a mod.");
							Player::OnConsoleMessage(currentPeer, "`oThere are so icy right i am so cold now!? (`oFrozen mod added! 30 Seconds Left`o)");
							static_cast<PlayerInfo*>(currentPeer->data)->frozen = true;
							static_cast<PlayerInfo*>(currentPeer->data)->freezetime = GetCurrentTimeInternalSeconds() + 30;
							Player::PlayAudio(currentPeer, "audio/freeze.wav", 0);
						}
						else
						{
							GamePacket p2 = packetEnd(appendIntx(appendString(createPacket(), "OnSetFreezeState"), 0));
							memcpy(p2.data + 8, &(((PlayerInfo*)(currentPeer->data))->netID), 4);
							ENetPacket* packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(currentPeer, 0, packet2);
							delete p2.data;
							static_cast<PlayerInfo*>(currentPeer->data)->skinColor = -2104114177;
							sendClothes(currentPeer);

							Player::OnTextOverlay(peer, "`2" + growid + " `1is unfrozen.");
							Player::OnTextOverlay(currentPeer, "`oYou have been `1unfrozen `oby a mod.");
							Player::OnConsoleMessage(currentPeer, "`oThere hot now need a new coffe for that! (`oFrozen mod over`o)");
							static_cast<PlayerInfo*>(currentPeer->data)->frozen = false;
							static_cast<PlayerInfo*>(currentPeer->data)->freezetime = 0;
						}
					}
					catch (const std::out_of_range& e) {
						std::cout << e.what() << std::endl;
					}
					found = true;
					break;
				}
			}
			if (!found)
			{
				Player::OnTextOverlay(peer, "`2" + growid + " `1is offline");
				return;
			}
		}
		else if (str.substr(0, 5) == "/ans ") {
			string ban_info = str;
			size_t extra_space = ban_info.find("  ");
			if (extra_space != std::string::npos)
			{
				ban_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string ban_user;
			string ban_time;
			if ((pos = ban_info.find(delimiter)) != std::string::npos)
			{
				ban_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oUsage: /ans <user> <answer text>"));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
				return;
			}
			if ((pos = ban_info.find(delimiter)) != std::string::npos)
			{
				ban_user = ban_info.substr(0, pos);
				ban_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oUsage: /ans <user> <answer text>"));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
				return;
			}
			ban_time = ban_info;
			string playerName = PlayerDB::getProperName(ban_user);
			string answerText = ban_time;
			bool sucanswered = false;
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName)
				{
					Player::OnConsoleMessage(currentPeer, "`9[`#" + server_name + " `#STAFF`9]`w: `3Administrator `2" + static_cast<PlayerInfo*>(peer->data)->rawName + "`3 just answered to your question`w:`2 " + answerText + "");
					Player::OnConsoleMessage(peer, "`2You successfully answered to `8" + playerName + "'s `2question.");
					sucanswered = true;

					string textInfo = "`1[M] `1[`o" + currentDateTime() + "`1] `6" + static_cast<PlayerInfo*>(peer->data)->tankIDName + " `4Just `2ANSWERD `8to `4player's `w" + playerName + " `6 question. `4The answer: `2 " + answerText + "";
					showModLogs(textInfo);

					break;
				}
			}
			if (!sucanswered)
			{
				Player::OnConsoleMessage(peer, "`4The player `2" + playerName + " `4 is not online.");
			}
		}
		else if (str == "/mods") {
			string x = "";
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isMod(currentPeer) && !static_cast<PlayerInfo*>(currentPeer->data)->isinv && !static_cast<PlayerInfo*>(currentPeer->data)->isNicked || isDev(peer) && isMod(currentPeer)) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->isNicked && isDev(peer)) {
						x.append("" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " (" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + ")`w, ");
					} else {
						x.append("" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`w, ");
					}
				}
			}
			x = x.substr(0, x.length() - 2);
			if (x == "") x = "(All are hidden)";
			Player::OnConsoleMessage(peer, "`oMods online: " + x);
			}
		else if (str.substr(0, 9) == "/copyset ") {
		string name = str.substr(9, cch.length() - 9 - 1);

		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;

			if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == PlayerDB::getProperName(name))
			{
				static_cast<PlayerInfo*>(peer->data)->cloth_hair = static_cast<PlayerInfo*>(currentPeer->data)->cloth_hair;
				static_cast<PlayerInfo*>(peer->data)->cloth_shirt = static_cast<PlayerInfo*>(currentPeer->data)->cloth_shirt;
				static_cast<PlayerInfo*>(peer->data)->cloth_pants = static_cast<PlayerInfo*>(currentPeer->data)->cloth_pants;
				static_cast<PlayerInfo*>(peer->data)->cloth_feet = static_cast<PlayerInfo*>(currentPeer->data)->cloth_feet;
				static_cast<PlayerInfo*>(peer->data)->cloth_face = static_cast<PlayerInfo*>(currentPeer->data)->cloth_face;
				static_cast<PlayerInfo*>(peer->data)->cloth_hand = static_cast<PlayerInfo*>(currentPeer->data)->cloth_hand;
				static_cast<PlayerInfo*>(peer->data)->cloth_back = static_cast<PlayerInfo*>(currentPeer->data)->cloth_back;
				static_cast<PlayerInfo*>(peer->data)->cloth_mask = static_cast<PlayerInfo*>(currentPeer->data)->cloth_mask;
				static_cast<PlayerInfo*>(peer->data)->cloth_necklace = static_cast<PlayerInfo*>(currentPeer->data)->cloth_necklace;
				static_cast<PlayerInfo*>(peer->data)->skinColor = static_cast<PlayerInfo*>(currentPeer->data)->skinColor;
				sendClothes(peer);
				Player::OnTextOverlay(peer, "`wYou Copied player `2" + ((PlayerInfo*)(currentPeer->data))->displayName + "`w clothes!");
				Player::PlayAudio(peer, "audio/change_clothes.wav", 0);
				break;
			}
		}
		}
		else if (str == "/game1start")
		{
		if (game1status == true)
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Oyun suanda oynaniyor. `2Sonra tekrar deneyin...");
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->currentWorld != "GAME1")
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Sadece bu komutu GAME1 world'unde kullanabilirsiniz.");
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->game1bet == false)
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Bir bahis yatirin. `2Bu komutu kullanin /game1bet <elmas> ");
			return;
		}
		int playersCount = 0;
		bool arok = true;
		ENetPeer* currentPeer;
		for (currentPeer = server->peers;
			currentPeer < &server->peers[server->peerCount];
			++currentPeer)
		{
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
			if (static_cast<PlayerInfo*>(currentPeer->data)->isinv == false)
			{
				if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "GAME1")
				{
					playersCount++;
					if (static_cast<PlayerInfo*>(currentPeer->data)->game1bet == false)
					{
						Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Oyunu baslatamadin, cunku `2" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " `4oyuna elmas yatirmadi.");
						Player::OnConsoleMessage(currentPeer, "`4`w[`2GAME1`8-`2DUYURU`w] `2" + static_cast<PlayerInfo*>(peer->data)->rawName + " `4oyunu baslatmaya calisti, ama haala oyun icin elmas yatirmadin. `2Kullan /game1bet <elmas>");
						arok = false;
						break;
					}
				}
			}
		}

		if (!arok) return;

		if (playersCount < 2)
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Burda sadece tek sen varsin. Oyunun baslamasi icin en az 2 kisi olmasi lazim.");
			return;
		}

		if (game1status == true)
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Zaten burada bir oyun var. `2Lutfen bekleyin...");
			return;
		}

		game1status = true;
		bool error = false;

		ENetPeer* currentPeer2;
		for (currentPeer2 = server->peers;
			currentPeer2 < &server->peers[server->peerCount];
			++currentPeer2)
		{
			if (currentPeer2->state != ENET_PEER_STATE_CONNECTED) continue;
			if (static_cast<PlayerInfo*>(currentPeer2->data)->isinv == false)
			{
				if (static_cast<PlayerInfo*>(currentPeer2->data)->currentWorld == "GAME1")
				{
					if (static_cast<PlayerInfo*>(currentPeer2->data)->game1bet == false)
					{
						error = true;
						break;
					}
					else
					{
						if (static_cast<PlayerInfo*>(currentPeer2->data)->canWalkInBlocks == true)
						{
							SendGhost(currentPeer2);
						}
						Player::OnConsoleMessage(currentPeer2, "`4`w[`2GAME1`8-`2BOT`w] `^Diken oyunu basladi. Tum betler: " + to_string(betamount) + " elmas! Katilimcilar: " + to_string(playersCount) + " oyuncu.");
						Player::PlayAudio(currentPeer2, "audio/ogg/battle_theme_loop.ogg", 0);
						Player::OnAddNotification(currentPeer2, "`2GAME1-DUYURU`w: `5GAME1 `1BASLADI! `4Hile kullanmayin ve ! Abuse yapmayin! `5Sadece eglenin...", "audio/already_used.wav", "interface/game_on.rttex");
					}

				}
			}
		}
		if (error)
		{
			game1status = false;
		}

		}
		else if (str.substr(0, 10) == "/game1bet ")
		{
		string imie = str.substr(10, cch.length() - 10 - 1);
		int gems = atoi(imie.c_str());
		int currentgems = 0;
		ifstream fs("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
		fs >> currentgems;
		fs.close();
		if (gems < 10000 || (static_cast<PlayerInfo*>(peer->data)->game1betgems + gems) > 1000000)
		{
			Player::OnConsoleMessage(peer, "`4Sadece bu kadar arasinda basabilirsin `210.000 `4- `21.000.000 `4elmas.");
			return;
		}
		if (gems > currentgems)
		{
			Player::OnConsoleMessage(peer, "`4O kadar elmasin yok.");
			return;
		}
		if (game1status == true)
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Zaten burada bir oyun var. `2Lutfen bekleyin...");
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->currentWorld != "GAME1")
		{
			Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`4 Sadece bu komutu GAME1 worldun'de kullanabilirsin..");
			return;
		}
		currentgems -= gems;
		GamePacket psa = packetEnd(appendInt(appendString(createPacket(), "OnSetBux"), currentgems));
		ENetPacket* packetsa = enet_packet_create(psa.data, psa.len, ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(peer, 0, packetsa);
		ofstream of("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
		of << currentgems;
		of.close();

		betamount += gems;
		static_cast<PlayerInfo*>(peer->data)->game1bet = true;
		static_cast<PlayerInfo*>(peer->data)->game1betgems += gems;
		int chancegetfromprize = 0;
		if (static_cast<PlayerInfo*>(peer->data)->game1betgems >= 10000 && static_cast<PlayerInfo*>(peer->data)->game1betgems < 30000) chancegetfromprize = 15;
		else if (static_cast<PlayerInfo*>(peer->data)->game1betgems >= 30000 && static_cast<PlayerInfo*>(peer->data)->game1betgems < 50000) chancegetfromprize = 30;
		else if (static_cast<PlayerInfo*>(peer->data)->game1betgems >= 50000 && static_cast<PlayerInfo*>(peer->data)->game1betgems < 100000) chancegetfromprize = 50;
		else if (static_cast<PlayerInfo*>(peer->data)->game1betgems >= 100000 && static_cast<PlayerInfo*>(peer->data)->game1betgems < 150000) chancegetfromprize = 75;
		else if (static_cast<PlayerInfo*>(peer->data)->game1betgems >= 150000 && static_cast<PlayerInfo*>(peer->data)->game1betgems < 250000) chancegetfromprize = 90;
		else chancegetfromprize = 100;

		Player::OnConsoleMessage(peer, "`4`w[`2GAME1`8-`2DUYURU`w]`2 Bahis kabul edildi. `7(Daha fazla istersen yatirabilirsin). `1Dunyadan sakin cikma. `wSuanda, maksimum `2" + to_string(chancegetfromprize) + "% `welmas kazanabilirsiniz (senin elmaslarin haric). `1Eger tum odul havuzunu toplamak istiyorsan, daha fazla bahis yatir.");

		}
		else if (str == "/food")
		{
		static_cast<PlayerInfo*>(peer->data)->food = 100;
		Player::OnTextOverlay(peer, "`2Aclik barin 100 oldu");
		}
		else if (str.substr(0, 6) == "/cure ")
		{
		string growid = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 1));
		bool found = false;
		ENetPeer* currentPeer2;
		for (currentPeer2 = server->peers;
			currentPeer2 < &server->peers[server->peerCount];
			++currentPeer2)
		{
			if (currentPeer2->state != ENET_PEER_STATE_CONNECTED) continue;
			if (static_cast<PlayerInfo*>(currentPeer2->data)->rawName == growid)
			{
				found = true;
				if (!static_cast<PlayerInfo*>(currentPeer2->data)->isHospitalized)
				{
					Player::OnTextOverlay(peer, "`4Hastanede degil `1hasta olan birinde dene.");
					return;
				}
				static_cast<PlayerInfo*>(currentPeer2->data)->cureprogress = 100;
				static_cast<PlayerInfo*>(currentPeer2->data)->isHospitalized = false;
				static_cast<PlayerInfo*>(currentPeer2->data)->isInHospitalBed = false;
				static_cast<PlayerInfo*>(currentPeer2->data)->food = 100;
				static_cast<PlayerInfo*>(currentPeer2->data)->cureprogress = 0;
				handle_world(currentPeer2, "BASLA");
				Player::OnTextOverlay(peer, "`2Iyilestirildi.");
				break;

				}
			}
			if(!found)
			{
				Player::OnTextOverlay(peer, "`4Aktif degil.");
				return;
			}
		} 
		else if (str == "/go") {
			if (static_cast<PlayerInfo*>(peer->data)->isCursed == true) return;
			if (static_cast<PlayerInfo*>(peer->data)->lastSbbWorld == "") {
				Player::OnTextOverlay(peer, "Unable to track down the location of the message.");
				return;
			} 
			if (static_cast<PlayerInfo*>(peer->data)->lastSbbWorld == static_cast<PlayerInfo*>(peer->data)->currentWorld) {
				Player::OnTextOverlay(peer, "Sorry, but you are already in the world!");
				return;
			}
			handle_world(peer, static_cast<PlayerInfo*>(peer->data)->lastSbbWorld);
		} 
		else if (str == "/online") {
			string online = "";
			int total = 0;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->adminLevel >= 0 && static_cast<PlayerInfo*>(currentPeer->data)->isinv == false) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->displayName == "" || static_cast<PlayerInfo*>(currentPeer->data)->rawName == "") continue;
					online += static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o ";
					total++;
				}
			}
			Player::OnConsoleMessage(peer, "`$Aktif oyuncular: (" + to_string(total) + "): `o" + online);
		}
		else if (str == "/kickall") {
			if (isMod(peer) || static_cast<PlayerInfo*>(peer->data)->rawName == world->owner || isWorldAdmin(peer, world)) {
				auto cooldownleft = calcBanDuration(static_cast<PlayerInfo*>(peer->data)->kickallcooldown);
				if (cooldownleft < 1) {
					static_cast<PlayerInfo*>(peer->data)->kickallcooldown = GetCurrentTimeInternalSeconds() + 6666;
					Player::OnConsoleMessage(peer, "`oWhen itis done. You can use `5/kickall `oagain after a ten minute cooldown period.");
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (isHere(peer, currentPeer)) {
							Player::PlayAudio(currentPeer, "audio/weird_hit.wav", 0);
							Player::OnConsoleMessage(currentPeer, "`oYou still have a cooldown. `5Please Wait!");
						}
					}
				}
				else
				{
					Player::OnConsoleMessage(peer, "`oYou still have a cooldown. `5Please Wait!");
					return;
				}
			}
			else {
				sendWrongCmd(peer);
				return;
			}
		}
		else if (str == "/pullall")
			{
				if (!isMod(peer)) return;
				if (world->width == 90 && world->height == 60) {
					Player::OnConsoleMessage(peer, "You can't use that command here.");
					return;
				}
				if (static_cast<PlayerInfo*>(peer->data)->job != "")
				{
					Player::OnTextOverlay(peer, "`oYou can't use that command, while you have the job!");
					return;
				}
				int howmuch = 0;
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) {
							continue;
						}
						if (!static_cast<PlayerInfo*>(currentPeer->data)->allowpull)
						{
							Player::OnConsoleMessage(currentPeer, static_cast<PlayerInfo*>(currentPeer->data)->displayName+" `4 is not allowed to pull him.");
							continue;
						}
						howmuch++;
						static_cast<PlayerInfo*>(currentPeer->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 2;
						static_cast<PlayerInfo*>(currentPeer->data)->disableanticheat = true;
						static_cast<PlayerInfo*>(currentPeer->data)->lastx = 0;
						static_cast<PlayerInfo*>(currentPeer->data)->lasty = 0;
						PlayerMoving data{};
						data.packetType = 0x0;
						data.characterState = 0x924;
						data.x = static_cast<PlayerInfo*>(peer->data)->x;
						data.y = static_cast<PlayerInfo*>(peer->data)->y;
						data.punchX = -1;
						data.punchY = -1;
						data.XSpeed = 0;
						data.YSpeed = 0;
						data.netID = static_cast<PlayerInfo*>(currentPeer->data)->netID;
						data.plantingTree = 0x0;
						SendPacketRaw(4, packPlayerMoving(&data), 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
						GamePacket p2 = packetEnd(appendFloat(appendString(createPacket(), "OnSetPos"), static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y));
						memcpy(p2.data + 8, &(static_cast<PlayerInfo*>(currentPeer->data)->netID), 4);
						ENetPacket* packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(currentPeer, 0, packet2);
						delete p2.data;
						if (isWorldOwner(peer, world)) Player::OnTextOverlay(currentPeer, "You were pulled by " + static_cast<PlayerInfo*>(peer->data)->displayName + "");
						else if (isMod(peer)) Player::OnTextOverlay(currentPeer, "You were summoned by a mod");
						break;
					}
				}
				Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`oYou have pulled `1" + to_string(howmuch) + " `oplayers.", 0, true);
			}
		else if (str.substr(0, 6) == "/pull ") {
		if (static_cast<PlayerInfo*>(peer->data)->rawName != world->owner && !isWorldAdmin(peer, world) && !isMod(peer)) return;
		if (world->width == 90 && world->height == 60) {
			Player::OnConsoleMessage(peer, "Kullanilmaz.");
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->job != "")
		{
			Player::OnTextOverlay(peer, "`oKullanilmaz!");
			return;
		}
		string pull_name = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 1));
		if (pull_name.size() < 3) {
			Player::OnConsoleMessage(peer, "Min 3 cumle yazin.");
			return;
		}
		bool Found = false, Block = false, notallowedtopull = false;
		int Same_name = 0, Sub_worlds_name = 0;
		string Intel_sense_nick = "";
		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
			if (isHere(peer, currentPeer)) {
				if (getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(pull_name)) != string::npos) Same_name++;
			}
			else if (isMod(peer)) {
				if (getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(pull_name)) != string::npos) Sub_worlds_name++;
			}
		}
		if (Same_name > 1) {
			Player::OnConsoleMessage(peer, "`oBurada o isimle baslayan 2 kisiden fazla var amk " + pull_name + " `oismin tamamini yaz bence!");
			return;
			}
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer) && getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(pull_name)) != string::npos) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) {
						Player::OnConsoleMessage(peer, "Ahhhhh!");
						Block = true;
						break;
					}
					if(!static_cast<PlayerInfo*>(currentPeer->data)->allowpull)
					{
						notallowedtopull = true;
						break;
					}
					static_cast<PlayerInfo*>(currentPeer->data)->disableanticheattime = GetCurrentTimeInternalSeconds() + 2;
					static_cast<PlayerInfo*>(currentPeer->data)->disableanticheat = true;
					static_cast<PlayerInfo*>(currentPeer->data)->lastx = 0;
					static_cast<PlayerInfo*>(currentPeer->data)->lasty = 0;

					Found = true;
					Intel_sense_nick = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
					PlayerMoving data{};
					data.packetType = 0x0;
					data.characterState = 0x924;
					data.x = static_cast<PlayerInfo*>(peer->data)->x;
					data.y = static_cast<PlayerInfo*>(peer->data)->y;
					data.punchX = -1;
					data.punchY = -1;
					data.XSpeed = 0;
					data.YSpeed = 0;
					data.netID = static_cast<PlayerInfo*>(currentPeer->data)->netID;
					data.plantingTree = 0x0;
					SendPacketRaw(4, packPlayerMoving(&data), 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
					GamePacket p2 = packetEnd(appendFloat(appendString(createPacket(), "OnSetPos"), static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y));
					memcpy(p2.data + 8, &(static_cast<PlayerInfo*>(currentPeer->data)->netID), 4);
					ENetPacket* packet2 = enet_packet_create(p2.data, p2.len, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet2);
					delete p2.data;
					if (isWorldOwner(peer, world)) Player::OnTextOverlay(currentPeer, "Bu kisi tarafindan pullandin " + static_cast<PlayerInfo*>(peer->data)->displayName + "");
					else if (isMod(peer)) Player::OnTextOverlay(currentPeer, "Yetkili tarafindan cekildin.");
					break;
				} 
			}
			if (Block) return;
			if(notallowedtopull)
			{
				Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`1(Misafirler konusamaz)", 0, true);
				return;
			}
			if (!Found && isMod(peer)) {
				if (Sub_worlds_name > 1) {
					Player::OnConsoleMessage(peer, "`oBu isimde bu dunyada birden fazla kisi var " + pull_name + " `obiraz daha biseyler yaz!");
					return;
				}
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(pull_name)) != string::npos) {
						if(!static_cast<PlayerInfo*>(currentPeer->data)->allowpull)
						{
							notallowedtopull = true;
							break;
						}
						Found = true;
						Intel_sense_nick = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
						handle_world(currentPeer, static_cast<PlayerInfo*>(peer->data)->currentWorld, false, false, "", true, static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y);
						Player::OnTextOverlay(currentPeer, "Bir yetkili tarafindan cekildin");
						break;
					}
				}
			}
			if(notallowedtopull)
			{
				Player::OnTextOverlay(peer, Intel_sense_nick+" `4pullayamazsin");
				return;
			}
			if (Found) {
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						Player::PlayAudio(currentPeer, "audio/object_spawn.wav", 0);
						Player::OnConsoleMessage(currentPeer, "`o" + static_cast<PlayerInfo*>(peer->data)->displayName + " `5yanina cekti `o" + Intel_sense_nick + "`o!");
					}
				}
			} else {
				if (isMod(peer)) {
					Player::OnConsoleMessage(peer, "`4Oops:`` There is nobody currently in this server with a name starting with `w" + pull_name + "``.");
				} else {
					Player::OnConsoleMessage(peer, "`4Oops:`` There is nobody currently in this world with a name starting with `w" + pull_name + "``.");
				}
			}
		}
		else if (str.substr(0, 4) == "/me ")
		{
			if (world->silence == true && !isWorldOwner(peer, world))
			{
				Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`1(Misafirler konusamaz)", 0, true);
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == false && static_cast<PlayerInfo*>(peer->data)->haveGrowId == true)
			{
				string namer = static_cast<PlayerInfo*>(peer->data)->displayName;
				GamePacket p2 = packetEnd(appendIntx(appendString(appendIntx(appendString(createPacket(), "OnTalkBubble"), static_cast<PlayerInfo*>(peer->data)->netID), "`#<`w" + namer + " `#" + str.substr(3, cch.length() - 3 - 1).c_str() + "`5>"), 0));
				ENetPacket* packet2 = enet_packet_create(p2.data,
					p2.len,
					ENET_PACKET_FLAG_RELIABLE);
				GamePacket p3 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`w<" + namer + " `#" + str.substr(3, cch.length() - 3 - 1).c_str() + "`w>"));
				ENetPacket* packet3 = enet_packet_create(p3.data,
					p3.len,
					ENET_PACKET_FLAG_RELIABLE);
				ENetPeer* currentPeer;
				for (currentPeer = server->peers;
					currentPeer < &server->peers[server->peerCount];
					++currentPeer)
				{
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
						continue;
					if (isHere(peer, currentPeer))
					{
						enet_peer_send(currentPeer, 0, packet2);
						enet_peer_send(currentPeer, 0, packet3);
					}
				}
				delete p2.data;
				delete p3.data;
				return;
			}
		}
		else if (str.substr(0, 6) == "/warn ") {
			string warn_info = str;
			size_t extra_space = warn_info.find("  ");
			if (extra_space != std::string::npos) warn_info.replace(extra_space, 2, " ");
			string delimiter = " ";
			size_t pos = 0;
			string warn_user;
			string warn_message;
			if ((pos = warn_info.find(delimiter)) != std::string::npos) warn_info.erase(0, pos + delimiter.length());
			else return;
			if ((pos = warn_info.find(delimiter)) != std::string::npos) {
				warn_user = warn_info.substr(0, pos);
				warn_info.erase(0, pos + delimiter.length());
			}
			else return;
			warn_message = warn_info;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == warn_user) {
					Player::OnAddNotification(currentPeer, "`wWarning from `4System`w: " + warn_message + "", "audio/hub_open.wav", "interface/atomic_button.rttex");
					Player::OnConsoleMessage(currentPeer, "`oWarning from `4System`o: " + warn_message + "");
					LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, PlayerDB::getProperName(warn_user), "Uyari: " + warn_message + "");
					break;
				}
			}
			Player::OnConsoleMessage(peer, "Warning sent (only works if the player is online)");
		}
		else if (str == "/rules") {
		std::ifstream news("rules.txt");
		std::stringstream buffer;
		buffer << news.rdbuf();
		std::string newsString(buffer.str());
		Player::OnDialogRequest(peer, newsString);
		}
		else if (str == "/oynat") {
		Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label|big|Bir sarki secin|right|\nadd_label|small|Ufak bir not, bu sarklar sunucu owner'leri tarafindan secilmis olup player onerilerine de aciktir eger bir isteginiz varsa discord sunucumuzdan istekte bulunabilirsiniz (Sarkiyi suanlik durduramazsiniz!)|left|\nadd_spacer|small|\nadd_checkbox|checkbox_ahrix|Muslum Gurses (Affet)|0|\nadd_checkbox|checkbox_phut|Tabi tabi (Sinan Akcil)|0|\nadd_checkbox|checkbox_stopme|Dalga (BATUFLEX)|0|\nadd_checkbox|checkbox_feelit|Kurtulus Kus & Burak Bulut (Baba Yak)|0|\nadd_checkbox|checkbox_sayso|Bilmem Ne! (Sefo)|0|\nadd_checkbox|checkbox_bealone|Limon Cicegim (Yigit Aktas)|0|\nadd_button|Iptal|Oynat|noflags|0|0|\nend_dialog|song_edit||");
		}
		else if (str.substr(0, 7) == "/spawn ")
		{
		if (world->name == "BEACHBLASTXX124XX654DD1" || world->name == "UNDERSEACROWSCREATORQWEQ" || world->name == "MARSBLASTCREATORQWE") return;
		//right same line player 
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//left same line player 
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 1


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 27, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 2


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 54, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 3


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 81, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);


		//up lr 4


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 108, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 5


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 135, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 6


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 162, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 7


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 189, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 8


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 216, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 9


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 243, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//up lr 10


		//right
		/*0*/ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y - 270, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//down lr 1

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 35, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 2

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 70, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 3

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 105, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 4

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 140, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 5

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 175, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 6

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 210, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 7

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 245, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 8

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 280, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 9

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 315, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);

		//down lr 10

		//right
		/* 0 */ sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 0 : 0)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 1 : -1)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 2 : -2)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 3 : -3)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 4 : -4)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 5 : -5)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 6 : -6)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 7 : -7)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 8 : -8)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 9 : -9)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? 10 : -10)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);




		//left
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -1 : 1)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -2 : 2)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -3 : 3)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -4 : 4)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -5 : 5)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -6 : 6)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -7 : 7)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -8 : 8)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -9 : 9)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);
		sendDrop(peer, -1, ((PlayerInfo*)(peer->data))->x + (32 * (((PlayerInfo*)(peer->data))->isRotatedLeft ? -10 : 10)), ((PlayerInfo*)(peer->data))->y + 350, atoi(str.substr(7, cch.length() - 7 - 1).c_str()), 1, 0);


		int block = atoi(str.substr(7, cch.length() - 7 - 1).c_str());

		Player::OnTextOverlay(peer, "You Spawned `2" + to_string(block) + "`o!");
		}
		else if (str.substr(0, 3) == "/r ") {
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true) return;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (currentPeer->data == nullptr) {
					SendConsole("currentPeer was nullptr", "ERROR");
					continue;
				}
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->lastMsger) {
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsger = static_cast<PlayerInfo*>(peer->data)->rawName;
					Player::OnConsoleMessage(peer, "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o)");
					Player::OnConsoleMessage(currentPeer, "CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`c) in [`o" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "`c] > `o" + str.substr(3, cch.length() - 3 - 1));
					Player::PlayAudio(currentPeer, "audio/pay_time.wav", 0);
					break;
				}
			}
		}
		else if (str.substr(0, 4) == "/rs ") {
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true) return;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (currentPeer->data == nullptr) {
					SendConsole("currentPeer was nullptr", "ERROR");
					continue;
				}
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->lastMsger) {
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsger = static_cast<PlayerInfo*>(peer->data)->rawName;
					Player::OnConsoleMessage(peer, "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o)");
					Player::OnConsoleMessage(currentPeer, "CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`c) in [`4<HIDDEN>`c] > `o" + str.substr(4, cch.length() - 4 - 1));
					Player::PlayAudio(currentPeer, "audio/pay_time.wav", 0);
					break;
				}
			}
		}
		else if (str == "/rgo") {
			if (static_cast<PlayerInfo*>(peer->data)->isCursed == true) return;
			if (static_cast<PlayerInfo*>(peer->data)->lastMsgWorld == "") {
				Player::OnTextOverlay(peer, "Unable to track down the location of the message.");
				return;
			} 
			if (static_cast<PlayerInfo*>(peer->data)->lastMsgWorld == static_cast<PlayerInfo*>(peer->data)->currentWorld) {
				Player::OnTextOverlay(peer, "Sorry, but you are already in the world!");
				return;
			}
			handle_world(peer, static_cast<PlayerInfo*>(peer->data)->lastMsgWorld);
		}
		else if(str == "/allowpull")
		{
			if(static_cast<PlayerInfo*>(peer->data)->allowpull)
			{
				static_cast<PlayerInfo*>(peer->data)->allowpull = false;
				Player::OnTextOverlay(peer, "You have prevented players to pull you.");
			}
			else
			{
				static_cast<PlayerInfo*>(peer->data)->allowpull = true;
				Player::OnTextOverlay(peer, "You have allowed players to pull you.");
			}
		}
		else if(str == "/allowwarpto")
		{
			if(static_cast<PlayerInfo*>(peer->data)->allowwarpto)
			{
				static_cast<PlayerInfo*>(peer->data)->allowwarpto = false;
				Player::OnTextOverlay(peer, "You have prevented players to warp to you.");
			}
			else
			{
				static_cast<PlayerInfo*>(peer->data)->allowwarpto = true;
				Player::OnTextOverlay(peer, "You have allowed players to warp to you.");
			}
		}
		else if (str == "/status") {
			Player::OnConsoleMessage(peer, "`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "````'s Durumu:");
			Player::OnConsoleMessage(peer, "Suanki World: `w" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "`` (`w" + to_string(static_cast<PlayerInfo*>(peer->data)->x / 32) + "``, `w" + to_string(static_cast<PlayerInfo*>(peer->data)->y / 32) + "``) (`w" + to_string(getPlayersCountInWorld(static_cast<PlayerInfo*>(peer->data)->currentWorld)) + "`` kisi) Envanter Bosluklari: `w" + to_string(static_cast<PlayerInfo*>(peer->data)->currentInventorySize) + "``");
			string visited = "";
			try {
				for (int i = 0; i < static_cast<PlayerInfo*>(peer->data)->lastworlds.size(); i++) {
					if (i == static_cast<PlayerInfo*>(peer->data)->lastworlds.size() - 1) {
						visited += "`#" + static_cast<PlayerInfo*>(peer->data)->lastworlds.at(i) + "``";
					} else {
						visited += "`#" + static_cast<PlayerInfo*>(peer->data)->lastworlds.at(i) + "``, ";
					}
				}
			} catch(const std::out_of_range& e) {
				std::cout << e.what() << std::endl;
			} 
			Player::OnConsoleMessage(peer, "Last visited: " + visited);
		}
		else if (str == "/stats") {
		int w_c = 0, s_c = 0, net_ = 1;
		const char* months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
		struct tm newtime;
		time_t now = time(0);
		localtime_s(&newtime, &now);
		string month = months[newtime.tm_mon];
		gamepacket_t p;
		p.Insert("OnConsoleMessage");
		p.Insert("S1 Uptime: 1 day, 8 hours, 6 mins, 17 secs - `$" + GetPlayerCountServer() + "`` players on.  Stats for this node: `$" + GetPlayerCountServer() + "`` players. (44 Android, 4 iOS) and `$129`` Worlds active. Server Load: 25.82 27.14 27.14``\n`2Growtopia Time (EDT/UTC-5): " + month + " " + to_string(newtime.tm_mday) + "th, " + to_string(newtime.tm_hour) + ":" + to_string(newtime.tm_min) + "");
		p.CreatePacket(peer);
	}
		else if (str == "/gc") {
		gamepacket_t p;
		p.Insert("OnConsoleMessage");
		p.Insert("`6>> Guildcast! Use /gc <message> to send messages to everyone who's online in your guild list. (they must have `5Show Guild Member Notifications`` checked to see them!)``");
		p.CreatePacket(peer);
	}
		else if (str == "/r") {
		gamepacket_t p;
		p.Insert("OnConsoleMessage");
		p.Insert("Usage: /r <`$your message``> - This will send a private message to the last person who sent you a message. Use /msg to talk to somebody new!");
		p.CreatePacket(peer);
	}
		else if (str.substr(0, 9) == "/setchat ") {
			string chatcode = (str.substr(9).c_str());
			if (chatcode.size() >= 2 || chatcode.size() <= 0) return;
			if (chatcode == "o") chatcode = "";
			static_cast<PlayerInfo*>(peer->data)->chatcolor = chatcode;
			if (chatcode != "") Player::OnConsoleMessage(peer, "`oYour chat color has been changed to `" + chatcode + "color`o!");
			else Player::OnConsoleMessage(peer, "`oYour chat color has been reverted to default!");
		}
		else if (str == "/stop") {										
			//Player::OnConsoleMessage(peer, "`@Threading is damaged, server is unable to stop properly");
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				GlobalMaintenance = true;
				Player::OnConsoleMessage(currentPeer, "`5Sunucu guncelleme icin bakim moduna geciyor.");
				Player::PlayAudio(currentPeer, "audio/boo_pke_warning_light.wav", 0);
				enet_peer_disconnect_later(currentPeer, 0);
			}
			SendConsole("Sakin sunucuyu kapatmayin dunyalar ve hesaplar kayitlaniyor...", "WARN");
			//saveAll();
		}
		else if (str == "/restart")
		{
			if (usedrestart) return;
			usedrestart = true;
			threads.push_back(std::thread(restart_manager));
		}
		else if(str == "/howgay")
		{
			int val = rand() % 100;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					Player::OnTalkBubble(currentPeer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName + " `ois `2" + to_string(val) + "% `wgay!", 0, true);
					Player::OnConsoleMessage(currentPeer, static_cast<PlayerInfo*>(peer->data)->displayName + " `ois `2" + to_string(val) + "% `wgay!");
				}
			}
		}
		else if (str.substr(0, 6) == "/give ") {
			string say_info = str;
			size_t extra_space = say_info.find("  ");
			if (extra_space != std::string::npos) say_info.replace(extra_space, 2, " ");
			string delimiter = " ";
			size_t pos = 0;
			string item_id;
			string item_count;
			if ((pos = say_info.find(delimiter)) != std::string::npos) {
				say_info.erase(0, pos + delimiter.length());
			} if ((pos = say_info.find(delimiter)) != std::string::npos) {
				item_id = say_info.substr(0, pos);
				say_info.erase(0, pos + delimiter.length());
			}
			item_count = say_info;
			if (item_id == "" && item_count != "") {
				bool contains_non_int2 = !std::regex_match(item_count, std::regex("^[0-9]+$"));
				if (contains_non_int2 == true) {
					return;
				}
				if (item_count.length() > 5) {
					Player::OnConsoleMessage(peer, "`oThis item does not exist");
					return;
				}
				int item_count_give = atoi(item_count.c_str());
				if (item_count_give > maxItems || item_count_give < 0 || item_count_give == 1424 || item_count_give == 5816) {
					Player::OnConsoleMessage(peer, "`oThis item does not exist");
					return;
				}
				if (CheckItemMaxed(peer, item_count_give, 1) || static_cast<PlayerInfo*>(peer->data)->inventory.items.size() + 1 >= static_cast<PlayerInfo*>(peer->data)->currentInventorySize && CheckItemExists(peer, item_count_give) && CheckItemMaxed(peer, item_count_give, 1) || static_cast<PlayerInfo*>(peer->data)->inventory.items.size() + 1 >= static_cast<PlayerInfo*>(peer->data)->currentInventorySize && !CheckItemExists(peer, item_count_give)) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " wont fit into my inventory!");
					return;
				}
				if (getItemDef(item_count_give).name.find("Subscription") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `ocan only be purchased for real gt diamond locks!");
					return;
				}
				if (getItemDef(item_count_give).name.find("Magplant") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `ooyunda maksimum arz 1000 daha fazla alinamaz!");
					return;
				}
				if (getItemDef(item_count_give).name.find("Blue gem") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `ouretilememesi oyun icin daha dogru olabilir!");
					return;
				}
				if (getItemDef(item_count_give).name.find("legen") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `osadece l-wiz'den uretilebililmelidir arz edilemez!!");
					return;
				}
				if (getItemDef(item_count_give).name.find("Diamond Builders") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `4Disabled item!");
					return;
				}
				if (getItemDef(item_count_give).name.find("The Dark Stone") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
					Player::OnConsoleMessage(peer, "`o" + getItemDef(item_count_give).name + " `4Only available in the gem store!");
					return;
				}

				Player::OnConsoleMessage(peer, "`oVerildi 1 '`$" + getItemDef(item_count_give).name + "`o'.");
				bool success = true;
				SaveItemMoreTimes(item_count_give, 1, peer, success);
				SendTradeEffect(peer, item_count_give, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->netID, 150);
				return;
			}
			bool contains_non_int2 = !std::regex_match(item_id, std::regex("^[0-9]+$"));
			if (contains_non_int2 == true) {
				return;
			}
			PlayerInventory inventory;
			if (item_id.length() > 5) {
				Player::OnConsoleMessage(peer, "`oThis item does not exist");
				return;
			}
			int item_id_give = atoi(item_id.c_str());
			if (item_id_give > maxItems || item_id_give < 0 || item_id_give == 1424 || item_id_give == 5816) {
				Player::OnConsoleMessage(peer, "`oThis item does not exist");
				return;
			}
			int item_count_give = 1;
			if (item_count != "") {
				bool contains_non_int2 = !std::regex_match(item_count, std::regex("^[0-9]+$"));
				if (contains_non_int2 == true) {
					return;
				}
				item_count_give = atoi(item_count.c_str());
				if (item_count_give > 250 || item_count_give <= 0) {
					Player::OnConsoleMessage(peer, "`oEsya verme sayisi 0 ile 250 arasinda olmalidir!");
					return;
				}
			}
			if (CheckItemMaxed(peer, item_id_give, item_count_give) || static_cast<PlayerInfo*>(peer->data)->inventory.items.size() + 1 >= static_cast<PlayerInfo*>(peer->data)->currentInventorySize && CheckItemExists(peer, item_id_give) && CheckItemMaxed(peer, item_id_give, item_count_give) || static_cast<PlayerInfo*>(peer->data)->inventory.items.size() + 1 >= static_cast<PlayerInfo*>(peer->data)->currentInventorySize && !CheckItemExists(peer, item_id_give)) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " wont fit into my inventory!");
				return;
			}
			if (getItemDef(item_id_give).name.find("Subscription") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `ocan only be purchased for real gt diamond locks!");
				return;
			}
			if (getItemDef(item_id_give).name.find("Magplant") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `ooyunda maksimum arz 1000 daha fazla alinamaz!");
				return;
			}
			if (getItemDef(item_id_give).name.find("Blue gem") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `ouretilememesi oyun icin daha dogru olabilir!");
				return;
			}
			if (getItemDef(item_id_give).name.find("legen") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `osadece l-wiz'den uretilebililmelidir arz edilemez!!");
				return;
			}
			if (getItemDef(item_id_give).name.find("Diamond Builders") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `4Disabled item!");
				return;
			}
			if (getItemDef(item_id_give).name.find("The Dark Stone") != string::npos && std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				Player::OnConsoleMessage(peer, "`o" + getItemDef(item_id_give).name + " `4Only available in the gem store!");
				return;
			}
			Player::OnConsoleMessage(peer, "`oVerildi " + to_string(item_count_give) + " '`$" + getItemDef(item_id_give).name + "`o'.");
			bool success = true;
			SaveItemMoreTimes(item_id_give, item_count_give, peer, success);
			SendTradeEffect(peer, item_id_give, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->netID, 150);
		}
		else if (str.substr(0, 10) == "/gemevent ") {
		//Player::OnConsoleMessage(peer, "Don't dare you to break my server economy");  
		//return;
		string multi = (str.substr(10).c_str());
		int num = atoi(multi.c_str());
		if (num < 0 || num > 5) {
			Player::OnConsoleMessage(peer, "Maksimum 5 kat elmas etkinligi baslatabilirsiniz!");
			return;
		}
		gem_multiplier = num;
		if (num == 0) {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				Player::OnConsoleMessage(currentPeer, static_cast<PlayerInfo*>(peer->data)->displayName + " `otum elmas etkinliklerini durdurdu!");
				Player::PlayAudio(currentPeer, "audio/loser.wav", 0);
			}
		}
		else {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				Player::OnConsoleMessage(currentPeer, static_cast<PlayerInfo*>(peer->data)->displayName + " `oelmas etkinligi baslatti " + to_string(num) + "x olarak devam ediyor!");
				Player::PlayAudio(currentPeer, "audio/levelup.wav", 0);
			}
		}
		}
		else if (str.substr(0, 6) == "/find ") {
			bool ScanNotFull = false;
			string item = (str.substr(6).c_str());
			vector<string> FoundItems;
			int foundkiek = 0;
			for (int i = 0; i < maxItems; i++) {
				string iname = getStrLower(getItemDef(i).name);
				if (iname.find(getStrLower(item)) != string::npos && !isSeed(i)) {
					if (foundkiek >= 100) {
						ScanNotFull = true;
						break;
					}
					foundkiek++;
					FoundItems.push_back("" + to_string(i) + ": " + getItemDef(i).name + "");
				}
			}
			Player::OnConsoleMessage(peer, "Eslesen itemler:");
			if (FoundItems.size() == 0) {
				Player::OnConsoleMessage(peer, "esya bulunamadi");
			} else {
				for (int i = 0; i < FoundItems.size(); i++) {
					Player::OnConsoleMessage(peer, FoundItems.at(i));
				}
			}
			if (ScanNotFull) Player::OnConsoleMessage(peer, "Aborting search because found over 100+ items");
		} 
		else if (str == "/regenerate") {
			threads.push_back(std::thread(restore_prices));
		} 
		/*else if (str == "/regeneratefull") {
			if (static_cast<PlayerInfo*>(peer->data)->rawName != "sebia") {
				sendWrongCmd(peer);
				return;
			}
			restore_prices_full();
		}*/
		else if (str == "/news") {
			std::ifstream news("news.txt");
			std::stringstream buffer;
			buffer << news.rdbuf();
			std::string newsString(buffer.str());
			Player::OnDialogRequest(peer, newsString);
		}
		else if (str == "/yetkibilgi") {
		std::ifstream news("rolbilgi.txt");
		std::stringstream buffer;
		buffer << news.rdbuf();
		std::string newsString(buffer.str());
		Player::OnDialogRequest(peer, newsString);
		}
		else if (str == "/logs")
		{
		GTDialog allLog;
		allLog.addLabelWithIcon("`wModerator & Developer Logs:", 1434, LABEL_SMALL);
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("gsmlogs", "`4Global System Message Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("spklogs", "`1Spk Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("nukelogs", "`5Nuke Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("curselogs", "`bCurse Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("banlogs", "`4BAN Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("mutelogs", "`9Mute Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("unmutelogs", "`5Un-mute Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("uncurselogs", "`5Un-curse Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("unbanlogs", "`5Un-ban Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("givelogs", "`5Give Item Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("givelevel", "`5Give Level Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("giveranklogs", "`5Give Rank Logs");
		allLog.addSpacer(SPACER_SMALL);
		allLog.addButton("givegems", "`5Give Gems Logs");
		allLog.addSpacer(SPACER_SMALL);

		allLog.addSpacer(SPACER_SMALL);
		allLog.addQuickExit();
		allLog.endDialog("Close", "", "Close it");
		Player::OnDialogRequest(peer, allLog.finishDialog());
		}
		else if (str == "/togglemod") {
			try {
				ifstream read_player("save/players/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".json");
				if (!read_player.is_open()) {
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				int adminLevel = j["adminLevel"];
				string nick = j["nick"];
				if (adminLevel < 1) {
					sendWrongCmd(peer);
					return;
				}
				if (static_cast<PlayerInfo*>(peer->data)->job != "")
				{
					Player::OnConsoleMessage(peer, "`oYou need to Quit your job first!");
					Player::OnTextOverlay(peer, "`oYou need to Quit your job first!");
					return;
				}
				if (static_cast<PlayerInfo*>(peer->data)->adminLevel < adminLevel) {
					Player::OnConsoleMessage(peer, "`oYou're now a mod! And people said becoming a mod was hard, ha!");
					static_cast<PlayerInfo*>(peer->data)->adminLevel = adminLevel;
					restore_player_name(world, peer);
					if (nick != "") {
						static_cast<PlayerInfo*>(peer->data)->isNicked = true;
						if (static_cast<PlayerInfo*>(peer->data)->NickPrefix != "") {
							static_cast<PlayerInfo*>(peer->data)->displayName = static_cast<PlayerInfo*>(peer->data)->NickPrefix + ". " + nick;
						} else {
							static_cast<PlayerInfo*>(peer->data)->displayName = nick;
						}
						static_cast<PlayerInfo*>(peer->data)->OriName = nick;
						for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
							if (isHere(peer, currentPeer)) {
								Player::OnNameChanged(currentPeer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName);
							}
						}
					}
				} else {
					Player::OnConsoleMessage(peer, "`oYou've lost your mod powers! This is more useful for testing and finding bugs as it's how most people will play. `5/togglemod `owill give them back, so don't be too sad");
					static_cast<PlayerInfo*>(peer->data)->adminLevel = 0;
					restore_player_name(world, peer);
				}
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		} 
		else if (str.substr(0, 5) == "/buy ") {
			if (static_cast<PlayerInfo*>(peer->data)->haveGrowId) {
				string itemFind = str.substr(5, cch.length() - 5 - 1);
				if (itemFind.length() < 3) {
					Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wEsya ismi cok kisa!``", 0, false);
					return;
				}
			SKIPFinds:;
				string itemLower2;
				vector<ItemDefinition> itemDefsfind;
				for (char c : itemFind) if (c < 0x20 || c > 0x7A) goto SKIPFinds;
				if (itemFind.length() < 3) goto SKIPFinds3;
				for (const ItemDefinition& item : itemDefs) {
					string itemLower;
					for (char c : item.name) if (c < 0x20 || c > 0x7A) goto SKIPFinds2;
					if (!(item.id % 2 == 0)) goto SKIPFinds2;
					itemLower2 = item.name;
					std::transform(itemLower2.begin(), itemLower2.end(), itemLower2.begin(), ::tolower);
					if (itemLower2.find(itemLower) != std::string::npos) {
						itemDefsfind.push_back(item);
					}
				SKIPFinds2:;
				}
			SKIPFinds3:;
				string listMiddle = "";
				string listFull = "";
				for (const ItemDefinition& item : itemDefsfind) {
					if (item.name != "") {
						string kys = item.name;
						std::transform(kys.begin(), kys.end(), kys.begin(), ::tolower);
						string kms = itemFind;
						std::transform(kms.begin(), kms.end(), kms.begin(), ::tolower);
						if (kys.find(kms) != std::string::npos) {
							int id = item.id;
							int itemid = item.id;
							if (getItemDef(itemid).blockType != BlockTypes::CLOTHING && getItemDef(itemid).rarity == 999 && itemid != 5142 && itemid != 1436 && itemid != 7188 && itemid != 3176 && itemid != 9528 || getItemDef(id).blockType == BlockTypes::LOCK && id != 7188 || id == 10034 || getItemDef(id).name.find("null") != string::npos || id == 10036 || getItemDef(id).name.find("Mooncake") != string::npos || getItemDef(id).name.find("Harvest") != string::npos && id != 1830 || getItemDef(id).name.find("Autumn") != string::npos || id == 1056 || id == 1804 || getItemDef(id).blockType == BlockTypes::COMPONENT || getItemDef(id).properties & Property_Chemical || id == 6920 || id == 6922 || id == 1874 || id == 1876 || id == 1904 || id == 1932 || id == 1900 || id == 1986 || id == 1996 || id == 2970 || id == 3140 || id == 3174 || id == 6028 || id == 6846 || id == 8962 || id == 2408 || id == 4428 || id == 5086 || id == 9240 || id == 9306 || id == 9290 || id == 7328 || id == 9416 || id == 10386 || id == 9410 || id == 1458 || id == 9408 || id == 9360 || id == 6866 || id == 6868 || id == 6870 || id == 6872 || id == 6874 || id == 6876 || id == 6878 || id == 2480 || id == 8452 || id == 5132 || id == 7166 || id == 5126 || id == 5128 || id == 5130 || id == 5144 || id == 5146 || id == 5148 || id == 5150 || id == 5162 || id == 5164 || id == 5166 || id == 5168 || id == 5180 || id == 5182 || id == 5184 || id == 5186 || id == 7168 || id == 7170 || id == 7172 || id == 7174 || id == 8834 || id == 7912 || id == 9212 || id == 5134 || id == 5152 || id == 5170 || id == 5188 || id == 980 || id == 9448 || id == 9310 || id == 10034 || id == 10036 || id == 8470 || id == 8286 || id == 6026 || id == 1784 || id == 9356 || id == 10022 || id == 902 || id == 10032 || id == 834 || id == 6 || id == 5640 || id == 9492 || id == 1782 || id == 9288 || id == 1780 || id == 8306 || id == 202 || id == 204 || id == 206 || id == 2950 || id == 4802 || id == 4994 || id == 5260 || id == 5814 || id == 5980 || id == 7734 || id == 2592 || id == 2242 || id == 1794 || id == 1792 || id == 778 || id == 9510 || id == 8774 || id == 2568 || id == 9512 || id == 9502 || id == 9482 || id == 2250 || id == 2248 || id == 2244 || id == 2246 || id == 2286 || id == 9508 || id == 9504 || id == 9506 || id == 274 || id == 276 || id == 9476 || id == 1486 || id == 4426 || id == 9496 || id == 278 || id == 9490 || id == 2410 || id == 9488 || id == 9452 || id == 9454 || id == 9472 || id == 9456 || id == 732 || id == 9458 || id == 6336 || id == 112 || id == 8 || id == 3760 || getItemDef(id).blockType == BlockTypes::FISH || id == 7372 || id == 9438 || id == 9462 || id == 9440 || id == 9442 || id == 9444 || id == 7960 || id == 7628 || id == 8552) continue;
							if (itemid == 9530 || itemid == 9526 || itemid == 9524 || itemid == 6312 || itemid == 998 || itemid == 986 || itemid == 992 || itemid == 990 || itemid == 996 || itemid == 988 || itemid == 1004 || itemid == 1006 || itemid == 1002 || itemid == 9504 || itemid == 9506 || itemid == 9508 || itemid == 9510 || itemid == 9512 || itemid == 9514 || itemid == 9518 || itemid == 9520 || itemid == 9502 || itemid == 9496 || itemid == 1790 || itemid == 9492 || itemid == 9494 || itemid == 9488 || itemid == 9222 || itemid == 1360 || itemid == 6260 || itemid == 822 || itemid == 1058 || itemid == 1094 || itemid == 1096 || itemid == 3402 || itemid == 1098 || itemid == 1828 || itemid == 3870 || itemid == 7058 || itemid == 1938 || itemid == 8452 || itemid == 1740 || itemid == 3040 || itemid == 5080 || itemid == 3100 || itemid == 1550 || itemid == 5740 || itemid == 3074 || itemid == 3010 || itemid == 8480 || itemid == 5084 || itemid == 10424 || itemid == 4656 || itemid == 7558 || itemid == 5082 || itemid == 1636 || itemid == 6008 || itemid == 4996 || itemid == 6416 || itemid == 2206 || itemid == 3792 || itemid == 3196 || itemid == 4654 || itemid == 3306 || itemid == 1498 || itemid == 1500 || itemid == 2804 || itemid == 2806 || itemid == 8270 || itemid == 8272 || itemid == 8274 || itemid == 2242 || itemid == 2244 || itemid == 2246 || itemid == 2248 || itemid == 2250 || itemid == 4676 || itemid == 4678 || itemid == 4680 || itemid == 4682 || itemid == 4652 || itemid == 4646 || itemid == 4648 || itemid == 4652 || itemid == 4650 || itemid == 10084 || itemid == 10086 || itemid == 9168 || itemid == 5480 || itemid == 4534 || itemid == 9166 || itemid == 9164 || itemid == 9162 || itemid == 9160 || itemid == 9158 || itemid == 9156 || itemid == 9154 || itemid == 9152 || itemid == 3008 || itemid == 3010 || itemid == 3040 || itemid == 5740 || itemid == 6254 || itemid == 6256 || itemid == 6258 || itemid == 6932 || itemid == 10262 || itemid == 10616 || itemid == 10582 || itemid == 10580 || itemid == 10664 || itemid == 10596 || itemid == 10598 || itemid == 10586 || itemid == 10590 || itemid == 10592 || itemid == 10576 || itemid == 10578 || itemid == 202 || itemid == 204 || itemid == 206 || itemid == 4994 || itemid == 2978 || itemid == 9268 || itemid == 5766 || itemid == 5768 || itemid == 5744 || itemid == 5756 || itemid == 5758 || itemid == 5760 || itemid == 5762 || itemid == 5754 || itemid == 7688 || itemid == 7690 || itemid == 7694 || itemid == 7686 || itemid == 7692 || itemid == 7698 || itemid == 7696 || itemid == 9286 || itemid == 9272 || itemid == 9290 || itemid == 9280 || itemid == 9282 || itemid == 9292 || itemid == 9284 || itemid == 362 || itemid == 3398 || itemid == 386 || itemid == 4422 || itemid == 364 || itemid == 9340 || itemid == 9342 || itemid == 9332 || itemid == 9334 || itemid == 9336 || itemid == 9338 || itemid == 366 || itemid == 2388 || itemid == 7808 || itemid == 7810 || itemid == 4416 || itemid == 7818 || itemid == 7820 || itemid == 5652 || itemid == 7822 || itemid == 7824 || itemid == 5644 || itemid == 390 || itemid == 7826 || itemid == 7830 || itemid == 9324 || itemid == 5658 || itemid == 3396 || itemid == 2384 || itemid == 5660 || itemid == 3400 || itemid == 4418 || itemid == 4412 || itemid == 388 || itemid == 3408 || itemid == 1470 || itemid == 3404 || itemid == 3406 || itemid == 2390 || itemid == 5656 || itemid == 2396 || itemid == 384 || itemid == 5664 || itemid == 4424 || itemid == 4400 || itemid == 1458 || itemid == 10660 || itemid == 10654 || itemid == 10632 || itemid == 10652 || itemid == 10626 || itemid == 10640 || itemid == 10662 || itemid == 574 || itemid == 592 || itemid == 760 || itemid == 900 || itemid == 766 || itemid == 1012 || itemid == 1272 || itemid == 1320 || itemid == 1540 || itemid == 1648 || itemid == 1740 || itemid == 1950 || itemid == 2900 || itemid == 1022 || itemid == 1030 || itemid == 1024 || itemid == 1032 || itemid == 1026 || itemid == 1028 || itemid == 1036 || itemid == 1034 || itemid == 2908 || itemid == 2974 || itemid == 3494 || itemid == 3060 || itemid == 3056 || itemid == 3052 || itemid == 3066 || itemid == 3048 || itemid == 3068 || itemid == 3166 || itemid == 2032 || itemid == 6780 || itemid == 754 || itemid == 794 || itemid == 796 || itemid == 2876 || itemid == 798 || itemid == 930 || itemid == 2204 || itemid == 2912 || itemid == 772 || itemid == 770 || itemid == 898 || itemid == 1582 || itemid == 1020 || itemid == 4132 || itemid == 3932 || itemid == 3934 || itemid == 4128 || itemid == 10246 || itemid == 4296 || itemid == 6212 || itemid == 1212 || itemid == 1190 || itemid == 1206 || itemid == 1166 || itemid == 1964 || itemid == 1976 || itemid == 1998 || itemid == 1946 || itemid == 2502 || itemid == 1958 || itemid == 1952 || itemid == 2030 || itemid == 3104 || itemid == 3112 || itemid == 3120 || itemid == 3092 || itemid == 3094 || itemid == 3096 || itemid == 4184 || itemid == 4178 || itemid == 4174 || itemid == 4180 || itemid == 4170 || itemid == 4168 || itemid == 4150 || itemid == 1180 || itemid == 1224 || itemid == 5226 || itemid == 5228 || itemid == 5230 || itemid == 5212 || itemid == 5246 || itemid == 5242 || itemid == 5234 || itemid == 7134 || itemid == 7118 || itemid == 7132 || itemid == 7120 || itemid == 7098 || itemid == 9018 || itemid == 9038 || itemid == 9026 || itemid == 9066 || itemid == 9058 || itemid == 9044 || itemid == 9024 || itemid == 9032 || itemid == 9036 || itemid == 9028 || itemid == 9030 || itemid == 9110 || itemid == 9112 || itemid == 10386 || itemid == 10326 || itemid == 10324 || itemid == 10322 || itemid == 10328 || itemid == 10316 || itemid == 1198 || itemid == 1208 || itemid == 1222 || itemid == 1250 || itemid == 1220 || itemid == 1202 || itemid == 1238 || itemid == 1168 || itemid == 1172 || itemid == 1230 || itemid == 1194 || itemid == 1192 || itemid == 1226 || itemid == 1196 || itemid == 1236 || itemid == 1182 || itemid == 1184 || itemid == 1186 || itemid == 1188 || itemid == 1170 || itemid == 1212 || itemid == 1214 || itemid == 1232 || itemid == 1178 || itemid == 1234 || itemid == 1250 || itemid == 1956 || itemid == 1990 || itemid == 1968 || itemid == 1960 || itemid == 1948 || itemid == 1966 || itemid == 3114 || itemid == 3118 || itemid == 3100 || itemid == 3122 || itemid == 3124 || itemid == 3126 || itemid == 3108 || itemid == 3098 || itemid == 1962 || itemid == 2500 || itemid == 4186 || itemid == 4188 || itemid == 4246 || itemid == 4248 || itemid == 4192 || itemid == 4156 || itemid == 4136 || itemid == 4152 || itemid == 4166 || itemid == 4190 || itemid == 4172 || itemid == 4182 || itemid == 4144 || itemid == 4146 || itemid == 4148 || itemid == 4140 || itemid == 4138 || itemid == 4142 || itemid == 5256 || itemid == 5208 || itemid == 5216 || itemid == 5218 || itemid == 5220 || itemid == 5214 || itemid == 5210 || itemid == 5254 || itemid == 5250 || itemid == 5252 || itemid == 5244 || itemid == 5236 || itemid == 7104 || itemid == 7124 || itemid == 7122 || itemid == 7102 || itemid == 7100 || itemid == 7126 || itemid == 7104 || itemid == 7124 || itemid == 7122 || itemid == 7102 || itemid == 7100 || itemid == 9048 || itemid == 9056 || itemid == 9034 || itemid == 1210 || itemid == 1216 || itemid == 1218 || itemid == 1992 || itemid == 1982 || itemid == 1994 || itemid == 1972 || itemid == 1980 || itemid == 1988 || itemid == 1984 || itemid == 3116 || itemid == 3102 || itemid == 3106 || itemid == 3110 || itemid == 4160 || itemid == 4162 || itemid == 4164 || itemid == 4154 || itemid == 4158 || itemid == 5224 || itemid == 5222 || itemid == 5232 || itemid == 5240 || itemid == 5248 || itemid == 5238 || itemid == 5256 || itemid == 7116 || itemid == 7108 || itemid == 7110 || itemid == 7128 || itemid == 7112 || itemid == 7130) continue;
							if (itemid == 6398 || itemid == 1436 || itemid == 6426 || itemid == 6340 || itemid == 6342 || itemid == 6350 || itemid == 6818 || itemid == 8244 || itemid == 8242 || itemid == 8240 || itemid == 8452 || itemid == 8454 || itemid == 8488 || itemid == 8498 || itemid == 8474 || itemid == 8476 || itemid == 8492 || itemid == 1498 || itemid == 1500 || itemid == 2804 || itemid == 2806 || itemid == 8270 || itemid == 8272 || itemid == 8274 || itemid == 3172 || itemid == 8478 || itemid == 8480 || itemid == 8486 || itemid == 8484 || itemid == 8482 || itemid == 8468 || itemid == 8494 || itemid == 8466 || itemid == 8490 || itemid == 8456 || itemid == 8458 || itemid == 8496 || itemid == 8472 || itemid == 5482 || itemid == 2240 || itemid == 3204 || itemid == 6114 || itemid == 4328 || itemid == 4326 || itemid == 4330 || itemid == 4324 || itemid == 4334 || itemid == 1242 || itemid == 1244 || itemid == 1246 || itemid == 1248 || itemid == 1282 || itemid == 1284 || itemid == 1286 || itemid == 1290 || itemid == 1288 || itemid == 1292 || itemid == 1294 || itemid == 1256 || itemid == 2586 || itemid == 782 || itemid == 3536 || itemid == 764 || itemid == 4176 || itemid == 4322 || itemid == 4080 || itemid == 2992 || itemid == 2976 || itemid == 3790 || itemid == 4990 || itemid == 1506 || itemid == 1274 || itemid == 9000 || itemid == 1252 || itemid == 8284 || itemid == 8954 || itemid == 8534 || itemid == 2386 || itemid == 4428 || itemid == 4426 || itemid == 5662 || itemid == 5642 || itemid == 5654 || itemid == 5646 || itemid == 5650 || itemid == 7828 || itemid == 7832 || itemid == 7834 || itemid == 9322 || itemid == 9344 || itemid == 9326 || itemid == 9316 || itemid == 9318 || itemid == 362 || itemid == 3398 || itemid == 386 || itemid == 4422 || itemid == 364 || itemid == 9340 || itemid == 9342 || itemid == 9332 || itemid == 9334 || itemid == 9336 || itemid == 9338 || itemid == 366 || itemid == 2388 || itemid == 7808 || itemid == 7810 || itemid == 4416 || itemid == 7818 || itemid == 7820 || itemid == 5652 || itemid == 7822 || itemid == 7824 || itemid == 5644 || itemid == 390 || itemid == 7826 || itemid == 7830 || itemid == 9324 || itemid == 5658 || itemid == 3396 || itemid == 2384 || itemid == 5660 || itemid == 3400 || itemid == 4418 || itemid == 4412 || itemid == 388 || itemid == 3408 || itemid == 1470 || itemid == 3404 || itemid == 3406 || itemid == 2390 || itemid == 5656 || itemid == 5648 || itemid == 2396 || itemid == 384 || itemid == 5664 || itemid == 4424 || itemid == 4400 || itemid == 9350 || itemid == 5040 || itemid == 5042 || itemid == 5044 || itemid == 392 || itemid == 3402 || itemid == 5032 || itemid == 5034 || itemid == 5036 || itemid == 5038 || itemid == 5018 || itemid == 5022 || itemid == 5060 || itemid == 5054 || itemid == 5058 || itemid == 5056 || itemid == 5050 || itemid == 5046 || itemid == 5052 || itemid == 5048 || itemid == 5070 || itemid == 5072 || itemid == 5074 || itemid == 5076 || itemid == 5066 || itemid == 5062 || itemid == 5068 || itemid == 5064 || itemid == 5080 || itemid == 5082 || itemid == 5084 || itemid == 5078 || itemid == 10236 || itemid == 10232 || itemid == 10194 || itemid == 10206 || itemid == 10184 || itemid == 10192 || itemid == 10190 || itemid == 10186 || itemid == 10212 || itemid == 10214 || itemid == 10216 || itemid == 10220 || itemid == 10222 || itemid == 10224 || itemid == 10226 || itemid == 10208 || itemid == 10210 || itemid == 10218 || itemid == 10196 || itemid == 10198 || itemid == 10250 || itemid == 10202 || itemid == 10204) continue;
							//food
							if(itemid == 4586 || itemid == 962 || itemid == 4564 || itemid == 4570 || itemid == 7188 || itemid == 868 || itemid == 196 || itemid == 4766 || itemid == 4568 || itemid == 4596 || itemid == 3472 || itemid == 7672 || itemid == 7056 || itemid == 4764 || itemid == 6316 || itemid == 676 || itemid == 874 || itemid == 4666 || itemid == 4594 || itemid == 4602 || itemid == 4604) continue;
							//robber things
							if (itemid == 3930 || itemid == 11462 || itemid == 944 || itemid == 7382 || itemid == 1786 || itemid == 11142 || itemid == 11140 || itemid == 11116 || itemid == 11232 || itemid == 11350 || itemid == 10914 || itemid == 5638 || itemid == 1538 || itemid == 7588 || itemid == 9498 || itemid == 1696 || itemid == 5142 || itemid == 11478 || itemid == 340 || itemid == 5666 || itemid == 4584) continue;
							listMiddle += "add_button_with_icon|tool" + to_string(item.id) + "|`$" + item.name + "``|left|" + to_string(item.id) + "||\n";
						}
					}
				}
				if (itemFind.length() < 3) {
					listFull = "add_textbox|`4Word is less then 3 letters!``|\nadd_spacer|small|\n";
					Player::OnDialogRequest(peer, "add_label_with_icon|big|`9Find item: " + itemFind + "``|left|3146|\n" + listFull + "add_textbox|Enter a word below to find the item|\nadd_text_input|item|Item Name||30|\nend_dialog|findid|Cancel|Find the item!|\n");
				} else if (itemDefsfind.size() == 0) {
					Player::OnDialogRequest(peer, "add_label_with_icon|big|`9Find item: " + itemFind + "``|left|3146|\n" + listFull + "add_textbox|Enter a word below to find the item|\nadd_text_input|item|Item Name||30|\nend_dialog|findid|Cancel|Find the item!|\n");
				} else {
					if (listMiddle.size() == 0) {
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wNo items were found with that name!``", 0, false);
					} else {
						Player::OnDialogRequest(peer, "add_label_with_icon|big|`wFound item : " + itemFind + "``|left|6016|\nadd_spacer|small|\nend_dialog|findid|Cancel|\nadd_spacer|big|\n" + listMiddle + "add_quick_exit|\n");
					}
				}
			}
		} 
		else if (str.substr(0, 5) == "/pay ") {
			if (static_cast<PlayerInfo*>(peer->data)->isCursed) {
				Player::OnConsoleMessage(peer, "You cannot perform this action while you are cursed");
				return;
			}
			std::ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
			std::string content((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
			int b = atoi(content.c_str());
			string imie = str.substr(5, cch.length() - 5 - 1);
			int phm = 0;
			if (imie.find(" ") != std::string::npos) {
				phm = atoi(imie.substr(imie.find(" ") + 1).c_str());
				imie = imie.substr(0, imie.find(" "));
			} else {
				return;
			}
			if (phm > 2147483647 || static_cast<PlayerInfo*>(peer->data)->rawName == PlayerDB::getProperName(imie) || static_cast<PlayerInfo*>(peer->data)->rawName == str.substr(5, cch.length() - 5 - 1) || phm < 0) {
				return;
			} else if (b >= phm) {
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == imie || static_cast<PlayerInfo*>(currentPeer->data)->displayName == imie) {
						std::ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						std::string acontent((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
						int a = atoi(acontent.c_str());
						int bb = b - phm;
						int aa = a + phm;
						ofstream myfile;
						myfile.open("save/gemdb/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						myfile << aa;
						myfile.close();
						myfile.open("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
						myfile << bb;
						myfile.close();
						Player::OnConsoleMessage(peer, "`oYou've sent `$" + to_string(phm) + " `ogems to `$" + imie + "`o!");
						Player::OnSetBux(peer, bb, 0);
						Player::OnSetBux(currentPeer, aa, 0);
						Player::OnConsoleMessage(currentPeer, "`oYou've received `$" + to_string(phm) + " `ogems from `$" + static_cast<PlayerInfo*>(peer->data)->displayName + "`o!");
						Player::OnAddNotification(currentPeer, "Player`w " + static_cast<PlayerInfo*>(peer->data)->displayName + "`o paid you `2" + std::to_string(phm) + " Gems`o!", "audio/piano_nice.wav", "interface/cash_icon_overlay.rttex");
						if (isDev(peer)) {
							LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, imie, "Received " + to_string(phm) + " gems (Suspicious)");
						}
						LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Paid " + to_string(phm) + " gems to " + imie + "");
						break;
					}
				}
			} else if (b < phm) {
				Player::OnConsoleMessage(peer, "`oYou don't have that much (`$" + to_string(phm) + "`o) gems to pay `$" + imie);
			}
		} 
		else if (str.substr(0, 7) == "/trade ") {
			if (static_cast<PlayerInfo*>(peer->data)->isCursed == true) {
				Player::OnConsoleMessage(peer, "`4You are cursed now!");
				return;
			}
			string trade_name = PlayerDB::getProperName(str.substr(7, cch.length() - 7 - 1));
			if (trade_name.size() < 3) {
				Player::OnConsoleMessage(peer, "You'll need to enter at least the first three characters of the person's name.");
				return;
			}
			bool Found = false, Block = false;
			int Same_name = 0, Sub_worlds_name = 0;
			string Intel_sense_nick = "";
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					if (getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(trade_name)) != string::npos) Same_name++;
				}
				else if (isMod(peer)) {
					if (getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(trade_name)) != string::npos) Sub_worlds_name++;
				}
			}
			if (Same_name > 1) {
				Player::OnConsoleMessage(peer, "`oThere are more than two players in this world starting with " + trade_name + " `obe more specific!");
				return;
			}
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer) && getStrLower(static_cast<PlayerInfo*>(currentPeer->data)->displayName).find(getStrLower(trade_name)) != string::npos) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) {
						Player::OnConsoleMessage(peer, "`oYou trade all your stuff to yourself in exchange for all your stuff.");
						Block = true;
						break;
					}
					Block = true;
					Found = true;
					Intel_sense_nick = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
					if (static_cast<PlayerInfo*>(currentPeer->data)->trade) {
						Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wThat person is busy.", 0, false);
						break;
					}
					static_cast<PlayerInfo*>(peer->data)->trade = false;
					static_cast<PlayerInfo*>(peer->data)->trade_netid = static_cast<PlayerInfo*>(currentPeer->data)->netID;
					Player::OnStartTrade(peer, static_cast<PlayerInfo*>(currentPeer->data)->displayName, static_cast<PlayerInfo*>(currentPeer->data)->netID);			
					break;
				}
			}
			if (Block) return;
			if (!Found) {
				Player::OnConsoleMessage(peer, "`4Oops:`` There is nobody currently in this world with a name starting with `w" + trade_name + "``.");
			}
		}
		else if (str == "/renderworld") {
			int space = 0;
			string append = "";
			for (auto i = 0; i < world->width * world->height; i++) {
				space += 32;
				append += "<div class='column' style='margin-left:" + to_string(space) + "px;position:absolute;'><img src='https://cdn.growstocks.xyz/item/" + to_string(world->items.at(i).foreground) + ".png'></div>";
				if ((space / 32) % world->width == 0 && i != 0) {
					append += "<br><br>";
					space = 0;
				}
			} ofstream write_map("save/render/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".html");
			if (!write_map.is_open()) {
				return;
			}
			write_map << append;
			write_map.close();
			Player::OnDialogRequest(peer, "set_default_color|`o\nadd_label_with_icon|big|`wWorld Render Share``|left|656|\nadd_textbox|`oYour world has been rendered!|\nadd_url_button||`wView``|NOFLAGS|http://" + server_ip + "/save/render/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld + ".html|Open picture in your browser?|0|0|\nend_dialog|1|Back||");
		}
		else if (str == "/help" || str == "/?") {
			string commands = "";
			for (int i = 0; i < role_commands.at(static_cast<PlayerInfo*>(peer->data)->adminLevel).size(); i++) {
				if (i + 1 == role_commands.at(static_cast<PlayerInfo*>(peer->data)->adminLevel).size()) {
					commands += "/" + role_commands.at(static_cast<PlayerInfo*>(peer->data)->adminLevel).at(i);
				} else {
					commands += "/" + role_commands.at(static_cast<PlayerInfo*>(peer->data)->adminLevel).at(i) + " ";
				}
			}
			if (static_cast<PlayerInfo*>(peer->data)->Subscriber) {
				for (int i = 0; i < sub_commands.size(); i++) {
					if (i + 1 == sub_commands.size()) {
						commands += "/" + sub_commands.at(i);
					} else {
						commands += "/" + sub_commands.at(i) + " ";
					}
				}
			}
			Player::OnConsoleMessage(peer, "`o>> Komutlar: " + commands);
			}
		else if (str == "/report") { //real growtopia reporting
		if (((PlayerInfo*)(peer->data))->currentWorld == "BASLA")
		{
			Player::OnTalkBubble(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`wBen bir orospu cocugu muyum?", 0, false);
		}
		else {
			cout << "[!] /report bildirildi " << ((PlayerInfo*)(peer->data))->displayName << endl;
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `2bir dunya scam raporu gonderdi lutfen /raporkayitlari yazarak detaylari ile beraber kontrol ediniz. `4" + world->name);
			send_logs(LogSType::REPORTLOG, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `4rapor bildirdi! `5rapor edilen dunya ismi  `b" + world->name + " `1lutfen acilen kontrol ediniz!");
			Player::OnDialogRequest(peer, "add_label_with_icon|big|`wBu dunyayi dolandiricilik olarak bildir|left|3732|\nadd_textbox|`oBu dunya oyunculari dolandirmak veya zorbalik yapmak icin kullaniliyorsa, isaretlemek icin `3Report`a basabilirsiniz. moderatorlerin kontrol etmesi icin.|\nadd_smalltext|`o- Bu ozellik oyunculari degil `2world`o raporlamak icindir. Eger dunya iyiyse ama insanlar uygun degilse, bunun yerine bir moda /msg gonderin.|\ nadd_smalltext|`o-Bu ozelligi kimlerin kullandigini kaydederiz.Yanlis raporlar gonderirseniz banlanirsiniz.|\nadd_smalltext|`o-Birden cok kez rapor vermek hicbir sey yapmaz - sadece bir kez rapor edin, dunya listemize girsin kontrol etmek icin.|\nadd_smalltext|`o-Raporu iptal etmenin bir yolu yok, bu yuzden dunyanin kotu oldugundan emin olmadikca rapor vermeyin!|\nadd-smalltext|`o- Nedeniyle ilgili 32 karakterlik kisa bir aciklama saglayin asagidaki dunyayi bildiriyorsunuz.|\nadd_text_input|reportworldtext|`oSebep: ||32|\nadd_textbox|`3Bu dunyayi bir aldatmaca olarak bildirmek istediginizden eminseniz, asagidaki Rapor'a basin!|\nend_dialog|sendworldreport|Iptal|Rapor|\n");
		}
		}
		else if (str.substr(0, 8) == "/warpto ") {
			if (static_cast<PlayerInfo*>(peer->data)->isCursed == true) return;
			if (str.substr(8, cch.length() - 8 - 1) == "") return;
			string name = str.substr(8, str.length());
			bool found = false;
			bool inExit = false;
			bool notallowed = false;

			if(static_cast<PlayerInfo*>(peer->data)->job != "")
			{
				Player::OnTextOverlay(peer, "`oYou can't use that command, while you have the job!");
				return;
			}
			if(world->name == "GAME1"){
				Player::OnConsoleMessage(peer, "You can't use that command here.");
				return;
			}

			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				string name2 = static_cast<PlayerInfo*>(currentPeer->data)->rawName;
				std::transform(name.begin(), name.end(), name.begin(), ::tolower);
				std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);
				if (name == name2) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT") {
						inExit = true;
						break;
					}
					if (!static_cast<PlayerInfo*>(currentPeer->data)->allowwarpto)
					{
						notallowed = true;
						break;
					}
					handle_world(peer, static_cast<PlayerInfo*>(currentPeer->data)->currentWorld, false, false, "", true, static_cast<PlayerInfo*>(currentPeer->data)->x, static_cast<PlayerInfo*>(currentPeer->data)->y);
					found = true;
				}
			}
			if (found) {
				Player::OnConsoleMessage(peer, "Teleportlaniliyor... " + name + "");
			}
			else if (inExit) {
				Player::OnConsoleMessage(peer, "" + name + " suanda bir dunyada degil.");
			}
			else if (notallowed) {
				Player::OnConsoleMessage(peer, "" + name + " teleportlanmaniz icin yetkiniz yok.");
			}
			else {
				Player::OnConsoleMessage(peer, "" + name + " suanda cevrimdisi.");
			}
		} 
		else if (str.substr(0, 6) == "/warp ") {
		if (static_cast<PlayerInfo*>(peer->data)->isCursed) return;
		if (world->name == "GAME1") {
			Player::OnConsoleMessage(peer, "You can't use that command here.");
			return;
		}
		string worldname = str.substr(6, str.length());
		toUpperCase(worldname);
		Player::OnTextOverlay(peer, "Magically warping to world `5" + worldname + "``...");
		handle_world(peer, worldname);
		}
		else if (str == "/nuke") {
		if (world->name == "START" || world->name == "SEHIRS" || world->name == "BASLA" || world->name == "GAME1" || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
		{
			Player::OnTextOverlay(peer, "`4OROSPU EVLADI SENI ANASINI SIKTIGIMIN EXPLOITCISI.");
			return;
		}
		if (world->isNuked) {
			world->isNuked = false;
			Player::OnTextOverlay(peer, "Dunya yasagi acildi");
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5isimli kisi su dunyanin yasagini kaldirdi `2" + world->name);
		}
		else {
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5isimli kisi bu worldu yasakladi `4" + world->name);
			send_logs(LogSType::NUKE, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oisimli sahis su worldu yasakladi `1dunya ismi `5" + world->name + "");

			world->isNuked = true;
			Player::OnTextOverlay(peer, "Dunya yasaklandi.");
			string name = static_cast<PlayerInfo*>(peer->data)->displayName;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				Player::OnConsoleMessage(currentPeer, "`o>>`4" + world->name + " `4dunyasi yasaklandi`o. Sadece guvende olun onu bilin istedik. Guzel oynayin, oyuncularimiz!");
				//Player::OnTextOverlay(currentPeer, "`o>>`4" + world->name + " `4dunyasi yasaklandi `oGuvenli oynayin `2growplay `ooyunculari!");
				Player::PlayAudio(currentPeer, "audio/bigboom.wav", 0);
				if (isHere(peer, currentPeer)) {
					if (!isMod(currentPeer)) {
						handle_world(currentPeer, "EXIT");
					}
					}
				}
			}
		}
		else if (str.substr(0, 5) == "/ssb ") {
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->lastSSB + 60000 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
			{
				static_cast<PlayerInfo*>(peer->data)->lastSSB = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
			}
			else
			{
				int kiekDar = (static_cast<PlayerInfo*>(peer->data)->lastSSB + 60000 - (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()) / 1000;
				Player::OnConsoleMessage(peer, "`9Cooldown `@Please Wait `9" + to_string(kiekDar) + " Seconds `@To Throw Another Broadcast!");
				return;
			}
			string sb_text = str.substr(4, cch.length() - 4 - 1);
			string name = static_cast<PlayerInfo*>(peer->data)->displayName;
			Player::OnConsoleMessage(peer, "`2>> `9Special Broadcast sent to all players online`2!");
			GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[SB]_ `5** `5from (`2" + name + "`5) in [`o" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "`5] ** :`$ " + sb_text));
			string text = "action|play_sfx\nfile|audio/double_chance.wav\ndelayMS|0\n";
			BYTE* data = new BYTE[5 + text.length()];
			BYTE zero = 0;
			int type = 3;
			memcpy(data, &type, 4);
			memcpy(data + 4, text.c_str(), text.length());
			memcpy(data + 4 + text.length(), &zero, 1);
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (!static_cast<PlayerInfo*>(currentPeer->data)->radio) continue;
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				ENetPacket* packet2 = enet_packet_create(data,
					5 + text.length(),
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet2);
				enet_peer_send(currentPeer, 0, packet);
				static_cast<PlayerInfo*>(currentPeer->data)->lastSbbWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
			}
			delete[] data;
			delete p.data;
			//elete p3.data;
	}
		else if (str.substr(0, 5) == "/ldc ") {
		if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
		{
			GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4Suanda mutelisin!"));
			ENetPacket* packet0 = enet_packet_create(p0.data,
				p0.len,
				ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet0);
			delete p0.data;
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->lastSSB + 100 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
		{
			static_cast<PlayerInfo*>(peer->data)->lastSSB = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
		}
		else
		{
			int kiekDar = (static_cast<PlayerInfo*>(peer->data)->lastSSB + 1 - (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()) / 1000;
			Player::OnConsoleMessage(peer, "`Bekle biraz kanks.");
			return;
		}
		string sb_text = str.substr(4, cch.length() - 4 - 1);
		string name = ((PlayerInfo*)(peer->data))->displayName;
		GamePacket p = packetEnd(appendInt(appendString(appendString(appendString(appendString(createPacket(), "OnAddNotification"), "interface/atomic_button.rttex"), " " + name + " `wLider Duyurusu`w atti`w: " + str.substr(4, cch.length() - 4 - 1).c_str()), "audio/double_chance.wav"), 0));
		string text = "action|play_sfx\nfile|audio/getpoint.wav\ndelayMS|0\n";
		BYTE* data = new BYTE[5 + text.length()];
		BYTE zero = 0;
		int type = 3;
		memcpy(data, &type, 4);
		memcpy(data + 4, text.c_str(), text.length());
		memcpy(data + 4 + text.length(), &zero, 1);
		ENetPeer* currentPeer;
		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
			if (!static_cast<PlayerInfo*>(currentPeer->data)->radio) continue;
			ENetPacket* packet = enet_packet_create(p.data,
				p.len,
				ENET_PACKET_FLAG_RELIABLE);
			ENetPacket* packet2 = enet_packet_create(data,
				5 + text.length(),
				ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(currentPeer, 0, packet2);
			enet_peer_send(currentPeer, 0, packet);
			static_cast<PlayerInfo*>(currentPeer->data)->lastSbbWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
		}
		delete[] data;
		delete p.data;
		//elete p3.data;
		}
		else if (str.substr(0, 5) == "/fakebanall ") {
		if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
		{
			GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4Suanda mutelisin!"));
			ENetPacket* packet0 = enet_packet_create(p0.data,
				p0.len,
				ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(peer, 0, packet0);
			delete p0.data;
			return;
		}
		if (static_cast<PlayerInfo*>(peer->data)->lastSSB + 100 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
		{
			static_cast<PlayerInfo*>(peer->data)->lastSSB = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
		}
		else
		{
			int kiekDar = (static_cast<PlayerInfo*>(peer->data)->lastSSB + 1111 - (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()) / 1000;
			Player::OnConsoleMessage(peer, "`Bekle biraz kanks.");
			return;
		}
		string sb_text = str.substr(4, cch.length() - 4 - 1);
		string name = ((PlayerInfo*)(peer->data))->displayName;
		GamePacket p = packetEnd(appendInt(appendString(appendString(appendString(appendString(createPacket(), "OnAddNotification"), "interface/atomic_button.rttex"), " " + name + " wWarning from `4System`w: You've been `4IP-BANNED " + str.substr(4, cch.length() - 4 - 1).c_str()), "audio/hub_open.wav"), 0));
		string text = "action|play_sfx\nfile|audio/hub_open.wav\ndelayMS|0\n";
		BYTE* data = new BYTE[5 + text.length()];
		BYTE zero = 0;
		int type = 3;
		memcpy(data, &type, 4);
		memcpy(data + 4, text.c_str(), text.length());
		memcpy(data + 4 + text.length(), &zero, 1);
		ENetPeer* currentPeer;
		for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
			if (!static_cast<PlayerInfo*>(currentPeer->data)->radio) continue;
			ENetPacket* packet = enet_packet_create(p.data,
				p.len,
				ENET_PACKET_FLAG_RELIABLE);
			ENetPacket* packet2 = enet_packet_create(data,
				5 + text.length(),
				ENET_PACKET_FLAG_RELIABLE);
			enet_peer_send(currentPeer, 0, packet2);
			enet_peer_send(currentPeer, 0, packet);
			static_cast<PlayerInfo*>(currentPeer->data)->lastSbbWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
		}
		delete[] data;
		delete p.data;
		//elete p3.data;
		}
		else if (str.substr(0, 5) == "/worldkick ")
		{
			if (world == nullptr || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || serverIsFrozen) return;
			if (!isMod(peer)) return;
			if (world->name == "START" || world->name == "SEHIRS" || world->name == "BASLA" || (world->name == "GAME1" && game1status) || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
			{
				Player::OnTextOverlay(peer, "You can't kick him in this world.");
				return;
			}
			string name = static_cast<PlayerInfo*>(peer->data)->displayName;
			string kickname = PlayerDB::getProperName(str.substr(5, cch.length() - 5 - 1));
			for (ENetPeer* currentPeerp = server->peers;
				currentPeerp < &server->peers[server->peerCount];
				++currentPeerp)
			{
				if (currentPeerp->state != ENET_PEER_STATE_CONNECTED)
					continue;

				if (isHere(peer, currentPeerp) && static_cast<PlayerInfo*>(currentPeerp->data)->rawName == kickname)
				{
					if (isDev(currentPeerp))
					{
						Player::OnConsoleMessage(peer, "`4You can't kick him!");
						break;
					}
					else if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == world->owner)
					{
						Player::OnConsoleMessage(peer, "`4You can't kick world owner!");
						break;
					}
					else
					{

						for (ENetPeer* currentPeer = server->peers;
							currentPeer < &server->peers[server->peerCount];
							++currentPeer)
						{
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
								continue;
							if (isHere(peer, currentPeer))
							{
								Player::OnConsoleMessage(currentPeer, name + " `4world kicks " + "`o" + kickname + " from `w" + world->name + "`o!");;
								Player::PlayAudio(currentPeer, "audio/boo_pke_warning_light.wav", 0);
							}
						}
						Player::OnConsoleMessage(peer, "`oYou've kicked `w" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o.");
						handle_world(currentPeerp, "EXIT");
						break;
					}
				}
			}
		}
		else if (str.substr(0, 5) == "/ban ")
		{
			if (world == nullptr || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || serverIsFrozen) return;
			if (static_cast<PlayerInfo*>(peer->data)->rawName == world->owner || isWorldAdmin(peer, world) || isMod(peer))
			{
				if (str.substr(5, cch.length() - 5 - 1) == "") return;
				if (static_cast<PlayerInfo*>(peer->data)->rawName == str.substr(5, cch.length() - 5 - 1)) return;
				if (world->name == "START" || world->name == "BASLA" || world->name == "SEHIRqwe" || (world->name == "GAME1" && game1status) || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
				{
					Player::OnTextOverlay(peer, "You can't ban him in this world.");
					return;
				}
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;
				string kickname = PlayerDB::getProperName(str.substr(5, cch.length() - 5 - 1));

				for (ENetPeer* currentPeerp = server->peers;
					currentPeerp < &server->peers[server->peerCount];
					++currentPeerp)
				{
					if (currentPeerp->state != ENET_PEER_STATE_CONNECTED)
						continue;
					
					if (isHere(peer, currentPeerp) && static_cast<PlayerInfo*>(currentPeerp->data)->rawName == kickname)
					{
						if (isDev(currentPeerp))
						{
							Player::OnConsoleMessage(peer, "`4You can't ban him!");
							break;
						}
						else if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == world->owner)
						{
							Player::OnConsoleMessage(peer, "`4You can't ban world owner!");
							break;
						}
						else
						{
							namespace fs = std::experimental::filesystem;
							if (!fs::is_directory("save/worldbans/_" + world->name) || !fs::exists("save/worldbans/_" + world->name))
							{
								fs::create_directory("save/worldbans/_" + world->name);
								std::ofstream outfile("save/worldbans/_" + world->name + "/" + static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
								outfile << "worldbanned by: " + static_cast<PlayerInfo*>(peer->data)->rawName;
								outfile.close();
							}
							else
							{
								std::ofstream outfile("save/worldbans/_" + world->name + "/" + static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
								outfile << "worldbanned by: " + static_cast<PlayerInfo*>(peer->data)->rawName;
								outfile.close();
							}

							for (ENetPeer* currentPeer = server->peers;
								currentPeer < &server->peers[server->peerCount];
								++currentPeer)
							{
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
									continue;
								if (isHere(peer, currentPeer))
								{
									Player::OnConsoleMessage(currentPeer, name + " `4world bans " + "`o" + kickname + " from `w" + world->name + "`o!");;
									Player::PlayAudio(currentPeer, "audio/repair.wav", 0);
								}
							}
							Player::OnConsoleMessage(peer, "`oYou've banned `w" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o. You can type `5/uba `oif you want unban him/her.");
							handle_world(currentPeerp, "EXIT");
							break;
						}
					}
				}
			}
		}
		else if (str == "/banall")
		{
			if (world == nullptr || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || serverIsFrozen) return;
			if (isMod(peer))
			{
				if (world->name == "START" || world->name == "JAIL" || world->name == "BASLA" || (world->name == "GAME1" && game1status) || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
				{
					Player::OnTextOverlay(peer, "You can't ban him in this world.");
					return;
				}
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;

				for (ENetPeer* currentPeerp = server->peers;
					currentPeerp < &server->peers[server->peerCount];
					++currentPeerp)
				{
					if (currentPeerp->state != ENET_PEER_STATE_CONNECTED)
						continue;

					if (isHere(peer, currentPeerp))
					{
						if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) {
							continue;
						}
						if (isDev(currentPeerp))
						{
							Player::OnConsoleMessage(peer, "`4You can't ban "+ static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
							break;
						}
						else if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == world->owner)
						{
							Player::OnConsoleMessage(peer, "`4You can't ban world owner!");
							break;
						}
						else
						{
							namespace fs = std::experimental::filesystem;
							if (!fs::is_directory("save/worldbans/_" + world->name) || !fs::exists("save/worldbans/_" + world->name))
							{
								fs::create_directory("save/worldbans/_" + world->name);
								std::ofstream outfile("save/worldbans/_" + world->name + "/" + static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
								outfile << "worldbanned by: " + static_cast<PlayerInfo*>(peer->data)->rawName;
								outfile.close();
							}
							else
							{
								std::ofstream outfile("save/worldbans/_" + world->name + "/" + static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
								outfile << "worldbanned by: " + static_cast<PlayerInfo*>(peer->data)->rawName;
								outfile.close();
							}

							for (ENetPeer* currentPeer = server->peers;
								currentPeer < &server->peers[server->peerCount];
								++currentPeer)
							{
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
									continue;
								if (isHere(peer, currentPeer))
								{
									Player::OnConsoleMessage(currentPeer, name + " `4world bans " + "`o" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o!");
									Player::PlayAudio(currentPeer, "audio/repair.wav", 0);
								}
							}
							Player::OnConsoleMessage(peer, "`oYou've banned `w" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o. You can type `5/uba `oif you want unban him/her.");
							handle_world(currentPeerp, "EXIT");
							break;
						}
					}
				}
			}
		}
		else if (str == "/worldkickall")
		{
			if (world == nullptr || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || serverIsFrozen) return;
			if (isMod(peer))
			{
				if (world->name == "START" || world->name == "SEHIRS" || world->name == "BASLA" || (world->name == "GAME1" && game1status) || world->name == "GAME1BACKUP" || world->name == "CEHENNEM")
				{
					Player::OnTextOverlay(peer, "You can't kick him in this world.");
					return;
				}
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;

				for (ENetPeer* currentPeerp = server->peers;
					currentPeerp < &server->peers[server->peerCount];
					++currentPeerp)
				{
					if (currentPeerp->state != ENET_PEER_STATE_CONNECTED)
						continue;

					if (isHere(peer, currentPeerp))
					{
						if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == static_cast<PlayerInfo*>(peer->data)->rawName) {
							continue;
						}
						if (isDev(currentPeerp))
						{
							Player::OnConsoleMessage(peer, "`4You can't kick " + static_cast<PlayerInfo*>(currentPeerp->data)->rawName);
							break;
						}
						else if (static_cast<PlayerInfo*>(currentPeerp->data)->rawName == world->owner)
						{
							Player::OnConsoleMessage(peer, "`4You can't kick world owner!");
							break;
						}
						else
						{
							
							for (ENetPeer* currentPeer = server->peers;
								currentPeer < &server->peers[server->peerCount];
								++currentPeer)
							{
								if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
									continue;
								if (isHere(peer, currentPeer))
								{
									Player::OnConsoleMessage(currentPeer, name + " `4world kicked " + "`o" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o!");
									Player::PlayAudio(currentPeer, "audio/boo_pke_warning_light.wav", 0);
								}
							}
							Player::OnConsoleMessage(peer, "`oYou've kicked `w" + static_cast<PlayerInfo*>(currentPeerp->data)->displayName + " `ofrom `w" + world->name + "`o.");
							handle_world(currentPeerp, "EXIT");
							break;
						}
					}
				}
			}
		}
		else if (str.substr(0, 9) == "/fakeban ") {
			string username = PlayerDB::getProperName(str.substr(8, cch.length() - 8 - 1));
			bool found = false;
			string userdisplay = "";
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == username) {
					userdisplay = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
					found = true;
					break;
				}
			}
			if (userdisplay == "") userdisplay = username;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				Player::OnConsoleMessage(currentPeer, "`#** `oThe Ancients have `4banned `w" + userdisplay + " `#** ``(`4/rules `oto see the rules!)");
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == username) {
					enet_peer_disconnect_later(currentPeer, 0);
				}
			}
			if (!found) {
				Player::OnConsoleMessage(peer, "`oPlayer was not found, so only notification was sent");
			} else {
				Player::OnConsoleMessage(peer, "`oUsed fake ban on " + userdisplay);
			}
		}
		else if (str.substr(0, 6) == "/pban ") {
			ifstream read_player("save/players/_" + PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6)) + ".json");
			if (!read_player.is_open()) {
				Player::OnConsoleMessage(peer, "Player does not exist!");
				return;
			}
			read_player.close();
			static_cast<PlayerInfo*>(peer->data)->last_ban_days = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_hours = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_minutes = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_reason = "";
			static_cast<PlayerInfo*>(peer->data)->lastInfo = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6));
			send_ban_panel(peer, "");
		}
		else if (str.substr(0, 5) == "/spk ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true) {
				Player::OnConsoleMessage(peer, "`4You are muted now!");
				return;
			}
			string say_info = str;
			size_t extra_space = say_info.find("  ");
			if (extra_space != std::string::npos) {
				say_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string say_user;
			string say_message;
			if ((pos = say_info.find(delimiter)) != std::string::npos) {
				say_info.erase(0, pos + delimiter.length());
			}
			else {
				Player::OnConsoleMessage(peer, "`oPlease enter a player's name.");
				return;
			}
			if ((pos = say_info.find(delimiter)) != std::string::npos) {
				say_user = say_info.substr(0, pos);
				say_info.erase(0, pos + delimiter.length());
			}
			else {
				Player::OnConsoleMessage(peer, "`oPlease enter a message.");
				return;
			}
			say_message = say_info;

			if (say_message[0] == '/')
			{
				Player::OnConsoleMessage(peer, "`4You cannot send commands for a player. Only chat messages.");
				return;
			}

			send_logs(LogSType::SPK, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oused /spk:`c " + say_user + " " + say_message);
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5used`1: `9/spk " + say_user + " " + say_message);
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == say_user) {
					Player::OnConsoleMessage(peer, "`$" + say_user + " `osaid >> `$" + say_message);
					SendChat(currentPeer, static_cast<PlayerInfo*>(currentPeer->data)->netID, say_message, world, cch);
					break;
				}
			}
		}
		else if (str.substr(0, 8) == "/unmute ")
		{
			if (str.substr(8, cch.length() - 8 - 1) == "") return;
			if (static_cast<PlayerInfo*>(peer->data)->rawName == str.substr(8, cch.length() - 8 - 1)) return;
			string name = PlayerDB::getProperName(str.substr(8, cch.length() - 8 - 1));
			try {
				ifstream read_player("save/players/_" + PlayerDB::getProperName(name) + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, "Player does not exist!");
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				string username = j["username"];
				int timemuted = j["timemuted"];
				if (timemuted == 0) {
					Player::OnConsoleMessage(peer, "" + PlayerDB::getProperName(name) + " is not muted");
					return;
				}
				j["timemuted"] = 0;
				ofstream write_player("save/players/_" + PlayerDB::getProperName(name) + ".json");
				write_player << j << std::endl;
				write_player.close();
				Player::OnConsoleMessage(peer, "Unmuted " + PlayerDB::getProperName(name));
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == name) {
						static_cast<PlayerInfo*>(currentPeer->data)->taped = false;
						static_cast<PlayerInfo*>(currentPeer->data)->isDuctaped = false;
						static_cast<PlayerInfo*>(currentPeer->data)->cantsay = false;
						static_cast<PlayerInfo*>(currentPeer->data)->lastMuted = 0;
						send_state(currentPeer);
						sendClothes(currentPeer);
						break;
					}
				}
				LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, PlayerDB::getProperName(name), "Unmuted");
				send_logs(LogSType::UNMUTE, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oUn-muted `2"+name);	
				send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5just Un-muted `4" + PlayerDB::getProperName(name));

			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
		else if (str.substr(0, 6) == "/mute ")
		{
			ifstream read_player("save/players/_" + PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6)) + ".json");
			if (!read_player.is_open()) {
				Player::OnConsoleMessage(peer, "Player does not exist!");
				return;
			}
			read_player.close();
			static_cast<PlayerInfo*>(peer->data)->muteall = false;
			static_cast<PlayerInfo*>(peer->data)->last_ban_days = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_hours = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_minutes = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_reason = "";
			static_cast<PlayerInfo*>(peer->data)->lastInfo = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6));
			send_mute_panel(peer, "");
		}
		else if (str.substr(0, 7) == "/curse ")
		{
			ifstream read_player("save/players/_" + PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6)) + ".json");
			if (!read_player.is_open()) {
				Player::OnConsoleMessage(peer, "Player does not exist!");
				return;
			}
			read_player.close();
			static_cast<PlayerInfo*>(peer->data)->curseall = false;
			static_cast<PlayerInfo*>(peer->data)->last_ban_days = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_hours = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_minutes = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_reason = "";
			static_cast<PlayerInfo*>(peer->data)->lastInfo = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 6));
			send_curse_panel(peer, "");
		}
		else if (str == "/muteall")
		{
			static_cast<PlayerInfo*>(peer->data)->muteall = true;
			static_cast<PlayerInfo*>(peer->data)->last_ban_days = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_hours = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_minutes = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_reason = "";
		send_mute_panel(peer, "");
		}
		else if (str == "/curseall")
		{
			static_cast<PlayerInfo*>(peer->data)->curseall = true;
			static_cast<PlayerInfo*>(peer->data)->last_ban_days = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_hours = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_minutes = 0;
			static_cast<PlayerInfo*>(peer->data)->last_ban_reason = "";
			send_curse_panel(peer, "");
		}
		else if (str == "/time")
		{
			sendTime(peer);
		}
		else if (str.substr(0, 9) == "/uncurse ")
		{
			if (str.substr(9, cch.length() - 9 - 1) == "") return;
			if (static_cast<PlayerInfo*>(peer->data)->rawName == str.substr(9, cch.length() - 9 - 1)) return;
			string cursename = str.substr(9, cch.length() - 9 - 1);
			try {
				ifstream read_player("save/players/_" + PlayerDB::getProperName(cursename) + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, "Player does not exist!");
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				string username = j["username"];
				int timecursed = j["timecursed"];
				if (timecursed == 0) {
					Player::OnConsoleMessage(peer, "" + PlayerDB::getProperName(cursename) + " is not cursed");
					return;
				}
				j["timecursed"] = 0;
				ofstream write_player("save/players/_" + PlayerDB::getProperName(cursename) + ".json");
				write_player << j << std::endl;
				write_player.close();
				Player::OnConsoleMessage(peer, "Uncursed " + PlayerDB::getProperName(cursename));
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == cursename) {
						static_cast<PlayerInfo*>(currentPeer->data)->skinColor = 0x8295C3FF;
						sendClothes(currentPeer);
						static_cast<PlayerInfo*>(currentPeer->data)->isCursed = false;
						static_cast<PlayerInfo*>(currentPeer->data)->lastCursed = 0;
						send_state(currentPeer);
					}
				}
				LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, PlayerDB::getProperName(cursename), "Uncursed");
				send_logs(LogSType::UNCURSE, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oUn-curse `2" + cursename);
				send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5just Un-cursed `4" + PlayerDB::getProperName(cursename));
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
		else if (str.substr(0, 7) == "/unban ")
		{
			string name_check = static_cast<PlayerInfo*>(peer->data)->rawName;
			toLowerCase(name_check);
			string name = PlayerDB::getProperName(str.substr(7, cch.length() - 7 - 1));
			try {
				ifstream read_player("save/players/_" + PlayerDB::getProperName(name) + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, "Player does not exist!");
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				string username = j["username"];
				int timebanned = j["timebanned"];
				bool isBanned = j["isBanned"];
				if (timebanned == 0 && !isBanned) {
					Player::OnConsoleMessage(peer, "" + PlayerDB::getProperName(name) + " is not banned");
					return;
				}
				j["timebanned"] = 0;
				j["isBanned"] = false;
				ofstream write_player("save/players/_" + PlayerDB::getProperName(name) + ".json");
				write_player << j << std::endl;
				write_player.close();
				Player::OnConsoleMessage(peer, "Unbanned " + PlayerDB::getProperName(name));
				LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, PlayerDB::getProperName(name), "Unbanned");
				send_logs(LogSType::UNBAN, "`4"+static_cast<PlayerInfo*>(peer->data)->rawName+" `oUn-banned `2"+PlayerDB::getProperName(name));
				send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5just Un-banned `4"+ PlayerDB::getProperName(name));
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
		else if (str.substr(0, 6) == "/msgs ")
		{
			bool found = false;
			if (static_cast<PlayerInfo*>(peer->data)->haveGrowId == false)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oTo prevent abuse, you `4must `obe `2registered `oin order to use this command!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			string msg_info = str;
			size_t extra_space = msg_info.find("  ");
			if (extra_space != std::string::npos)
			{
				msg_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string pm_user;
			string pm_message;
			if ((pos = msg_info.find(delimiter)) != std::string::npos)
			{
				msg_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oPlease specify a `2player `oyou want your message to be delivered to."));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
			}
			if ((pos = msg_info.find(delimiter)) != std::string::npos)
			{
				pm_user = msg_info.substr(0, pos);
				msg_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oPlease enter your `2message`o."));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
			}
			pm_message = msg_info;
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->msgName == PlayerDB::getProperName(pm_user))
				{
					if (static_cast<PlayerInfo*>(currentPeer->data)->isinv == true) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->isNicked == true) continue;
					/*if (static_cast<PlayerInfo*>(currentPeer->data)->isDisableMessages == true)
					{
						Player::OnConsoleMessage(peer, "`oThis player disabled private messages. Try it later.");
						continue;
					}*/
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsger = static_cast<PlayerInfo*>(peer->data)->rawName;
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsgerTrue = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsgWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
					GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o)"));
					ENetPacket* packet0 = enet_packet_create(p0.data,
						p0.len,
						ENET_PACKET_FLAG_RELIABLE);
					GamePacket p10 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o) `o(`4Note: `oMessage a mod `4ONLY ONCE `oabout an issue. Mods don't fix scams or replace items, they punish players who break the `5/rules`o.)"));
					ENetPacket* packet10 = enet_packet_create(p10.data,
						p10.len,
						ENET_PACKET_FLAG_RELIABLE);
					if (isMod(currentPeer) && !isMod(peer) && static_cast<PlayerInfo*>(currentPeer->data)->isNicked == false)
					{
						enet_peer_send(peer, 0, packet10);
					}
					else
					{
						enet_peer_send(peer, 0, packet0);
					}
					delete p0.data;
					delete p10.data;
					found = true;
					GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`c) in [`4<HIDDEN>`c] > `o" + pm_message));
					string text = "action|play_sfx\nfile|audio/pay_time.wav\ndelayMS|0\n";
					BYTE* data = new BYTE[5 + text.length()];
					BYTE zero = 0;
					int type = 3;
					memcpy(data, &type, 4);
					memcpy(data + 4, text.c_str(), text.length());
					memcpy(data + 4 + text.length(), &zero, 1);
					ENetPacket* packet2 = enet_packet_create(data,
						5 + text.length(),
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet2);
					delete[] data;
					ENetPacket* packet = enet_packet_create(ps.data,
						ps.len,
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet);
					delete ps.data;
					break;
				}
			}
			if (found == false)
			{
				Player::OnConsoleMessage(peer, "`6>> No one online who has a name starting with " + pm_user + "`8.");
			}
		}
		else if (str.substr(0, 5) == "/msg ")
		{
			bool found = false;
			if (static_cast<PlayerInfo*>(peer->data)->haveGrowId == false)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oTo prevent abuse, you `4must `obe `2registered `oin order to use this command!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			string msg_info = str;
			size_t extra_space = msg_info.find("  ");
			if (extra_space != std::string::npos)
			{
				msg_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string pm_user;
			string pm_message;
			if ((pos = msg_info.find(delimiter)) != std::string::npos)
			{
				msg_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oPlease specify a `2player `oyou want your message to be delivered to."));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
			}
			if ((pos = msg_info.find(delimiter)) != std::string::npos)
			{
				pm_user = msg_info.substr(0, pos);
				msg_info.erase(0, pos + delimiter.length());
			}
			else
			{
				GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`oPlease enter your `2message`o."));
				ENetPacket* packet = enet_packet_create(ps.data,
					ps.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete ps.data;
			}
			pm_message = msg_info;
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->msgName == PlayerDB::getProperName(pm_user))
				{
					if (static_cast<PlayerInfo*>(currentPeer->data)->isinv == true) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->isNicked == true) continue;
					/*if (static_cast<PlayerInfo*>(currentPeer->data)->isDisableMessages == true)
					{
						Player::OnConsoleMessage(peer, "`oThis player disabled private messages. Try it later.");
						continue;
					}*/
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsger = static_cast<PlayerInfo*>(peer->data)->rawName;
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsgerTrue = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
					static_cast<PlayerInfo*>(currentPeer->data)->lastMsgWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
					GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o)"));
					ENetPacket* packet0 = enet_packet_create(p0.data,
						p0.len,
						ENET_PACKET_FLAG_RELIABLE);
					GamePacket p10 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`o) `o(`4Note: `oMessage a mod `4ONLY ONCE `oabout an issue. Mods don't fix scams or replace items, they punish players who break the `5/rules`o.)"));
					ENetPacket* packet10 = enet_packet_create(p10.data,
						p10.len,
						ENET_PACKET_FLAG_RELIABLE);
					if (isMod(currentPeer) && !isMod(peer) && static_cast<PlayerInfo*>(currentPeer->data)->isNicked == false)
					{
						enet_peer_send(peer, 0, packet10);
					}
					else
					{
						enet_peer_send(peer, 0, packet0);
					}
					delete p0.data;
					delete p10.data;
					found = true;
					GamePacket ps = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`c) in [`o" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "`c] > `o" + pm_message));
					string text = "action|play_sfx\nfile|audio/pay_time.wav\ndelayMS|0\n";
					BYTE* data = new BYTE[5 + text.length()];
					BYTE zero = 0;
					int type = 3;
					memcpy(data, &type, 4);
					memcpy(data + 4, text.c_str(), text.length());
					memcpy(data + 4 + text.length(), &zero, 1);
					ENetPacket* packet2 = enet_packet_create(data,
						5 + text.length(),
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet2);
					delete[] data;
					ENetPacket* packet = enet_packet_create(ps.data,
						ps.len,
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet);
					delete ps.data;
					break;
				}
			}
			if (found == false)
			{
				Player::OnConsoleMessage(peer, "`6>> No one online who has a name starting with " + pm_user + "`8.");
			}
		}
		else if (str == "/uba")
		{
			if (world == nullptr || static_cast<PlayerInfo*>(peer->data)->currentWorld == "EXIT" || serverIsFrozen) return;
			if (isWorldAdmin(peer, world) || static_cast<PlayerInfo*>(peer->data)->rawName == world->owner || isMod(peer))
			{
				namespace fs = std::experimental::filesystem;
				fs::remove_all("save/worldbans/_" + static_cast<PlayerInfo*>(peer->data)->currentWorld);
				Player::OnConsoleMessage(peer, "`oYou unbanned everyone from the world!");
			}
		}
		else if (str.substr(0, 5) == "/eff ") {
			Player::OnConsoleMessage(peer, "`oEffect spawned (" + str.substr(5, cch.length() - 5 - 1) + ")");
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					Player::OnParticleEffect(currentPeer, atoi(str.substr(5, cch.length() - 5 - 1).c_str()), static_cast<PlayerInfo*>(peer->data)->x, static_cast<PlayerInfo*>(peer->data)->y, 0);
				}
			}
		}
		else if (str.substr(0, 3) == "/p ") {
			Player::OnConsoleMessage(peer, "`oPunch Effect changed to " + str.substr(3, cch.length() - 3 - 1));
			static_cast<PlayerInfo*>(peer->data)->effect = atoi(str.substr(3, cch.length() - 3 - 1).c_str());
			sendPuncheffect(peer, static_cast<PlayerInfo*>(peer->data)->effect);
			send_state(peer); 
		}
		else if (str.substr(0, 5) == "/gsm ")
		{
		send_logs(LogSType::GSM, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `owrote:`c " + str.substr(4, cch.length() - 4 - 1));
		send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5used`1: `9/gsm " + str.substr(4, cch.length() - 4 - 1));
		string name = static_cast<PlayerInfo*>(peer->data)->displayName;
		GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4Genel Sistem Mesaji: `o" + str.substr(4, cch.length() - 4 - 1)));
		string text = "action|play_sfx\nfile|audio/sungate.wav\ndelayMS|0\n";
			BYTE* data = new BYTE[5 + text.length()];
			BYTE zero = 0;
			int type = 3;
			memcpy(data, &type, 4);
			memcpy(data + 4, text.c_str(), text.length());
			memcpy(data + 4 + text.length(), &zero, 1);
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (!static_cast<PlayerInfo*>(currentPeer->data)->radio)
					continue;
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet);
				ENetPacket* packet2 = enet_packet_create(data,
					5 + text.length(),
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet2);
				//enet_host_flush(server);
			}
			delete[] data;
			delete p.data;
		}
		else if (str.substr(0, 9) == "/unbanip ")
		{
			string playerCalled = PlayerDB::getProperName(str.substr(9, cch.length() - 9 - 1));

			string ipid;
			string getmac;
			string getrid;
			string getsid;
			string getgid;
			string getvid;
			string getaid;
			string getip;

			try {
				ifstream read_player("save/players/_" + PlayerDB::getProperName(playerCalled) + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, "Player does not exist!");
					return;
				}		
				json j;
				read_player >> j;
				read_player.close();
				string username = j["username"];
				string ipid = j["ipID"];
				string getmac = j["mac"];
				string getrid = j["rid"];
				string getsid = j["sid"];
				string getgid = j["gid"];
				string getvid = j["vid"];
				string getaid = j["aid"];
				string getip = j["ip"];
				string isipbanned = "No.";
				string macremoved = getmac;
				Remove(macremoved, ":");
				bool existx = std::experimental::filesystem::exists("save/ipbans/mac/" + macremoved + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/mac/" + macremoved + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/rid/" + getrid + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/rid/" + getrid + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/gid/" + getgid + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/gid/" + getgid + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/ip/" + getip + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/ip/" + getip + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/sid/" + getsid + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/sid/" + getsid + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/aid/" + getaid + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/aid/" + getaid + ".txt").c_str());
				}
				existx = std::experimental::filesystem::exists("save/ipbans/ip_id/" + ipid + ".txt");
				if (existx) {
					isipbanned = "Yes.";
					remove(("save/ipbans/ip_id/" + ipid + ".txt").c_str());
				}
				if (isipbanned == "No.") {
					Player::OnConsoleMessage(peer, username + " is not ip banned!");
				} else {
					Player::OnConsoleMessage(peer, username + " ip ban was removed!");
					send_logs(LogSType::UNBAN, "`4"+static_cast<PlayerInfo*>(peer->data)->rawName+" `ounbaned ip `4"+PlayerDB::getProperName(playerCalled));
				}
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
		else if (str.substr(0, 8) == "/infoex ") {
			string checknick = PlayerDB::getProperName(str.substr(8, cch.length() - 8 - 1));
			if(checknick == "crows"  || checknick == "rooster" || checknick == "junix") return;
			try {
				ifstream read_player("save/players/_" + checknick + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, checknick + " u can't check creator Crows infoex!");
					return;
				}
				ifstream ifsz("save/gemdb/_" + checknick + ".txt");
				string gemscount((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));

				json j;
				read_player >> j;
				read_player.close();
				string username = j["username"];
				int playerid = j["playerid"];
				string email = j["email"];
				string ipID = j["ipID"];
				int receivedwarns = j["receivedwarns"];
				string mac = j["mac"];
				string rid = j["rid"];
				string sid = j["sid"];
				string gid = j["gid"];
				string vid = j["vid"];
				string aid = j["aid"];
				string ip = j["ip"];
				int adminLevel = j["adminLevel"];
				string subtype = j["subtype"];
				string subdate = j["subdate"];
				string nick = j["nick"];
				int level = j["level"];
				bool isbanned = j["isBanned"];
				string stringIsBanned = isbanned ? "yes" : "no";
				if (mac == "02:00:00:00:00:00") mac = "N/A";
				if (rid == "" || rid == "none") rid = "N/A";
				if (sid == "" || sid == "none") sid = "N/A";
				if (gid == "" || gid == "none") gid = "N/A";
				if (vid == "" || vid == "none") vid = "N/A";
				if (aid == "" || aid == "none") aid = "N/A";
				if (ip == "127.0.0.1") ip = "localhost";
				if (ip == "" || ip == "none") ip = "N/A";
				Player::OnConsoleMessage(peer, "Found 1 matches:");
				Player::OnConsoleMessage(peer, username + " (ID: " + to_string(playerid) + ") IP: " + ip + ", Mac: " + mac + ", RID: " + rid + ", SID: " + sid + ", GID: " + gid + ", VID: " + vid + ", AID: " + aid + "");
				if (nick == "") nick = "N/A";
				string status = role_nameko.at(adminLevel);
				if (subdate == "") subdate = "N/A";
				Player::OnConsoleMessage(peer, "Status: " + status + ", Nickname: " + nick + ", Email: " + email + ", Level: " + to_string(level) + ", Subscription: " + subdate + ", Gems: "+gemscount+", isBanned: "+stringIsBanned+"");
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}	
		}
		else if (str.substr(0, 6) == "/info ") {
			SendPunishView(peer, PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 1)));
		}
		else if (str.substr(0, 7) == "/banip ")
		{
			string nick = PlayerDB::getProperName(str.substr(7, cch.length() - 7 - 1));
			if (nick == "crows" || nick == "who1sleyla" || nick == "rooster") {
				autoBan(peer, true, 100000, "ip ban wrench abuse on " + nick);
				return;
			}
			Player::OnTextOverlay(peer, "IP Ban mod applied to " + nick + "!");
			try {
				ifstream read_player("save/players/_" + nick + ".json");
				if (!read_player.is_open()) {
					return;
				}
				json j;
				read_player >> j;
				read_player.close();
				j["isBanned"] = true;
				string registermac = j["mac"];
				string registerrid = j["rid"];
				string registersid = j["sid"];
				string registergid = j["gid"];
				string registervid = j["vid"];
				string registeraid = j["aid"];
				string registerIP = j["ip"];
				if (registermac != "02:00:00:00:00:00" && registermac != "" && registermac != "none") {
					Remove(registermac, ":");
					std::ofstream outfile2("save/ipbans/mac/" + registermac + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registerrid != "" && registerrid != "none") {
					std::ofstream outfile2("save/ipbans/rid/" + registerrid + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registersid != "" && registersid != "none") {
					std::ofstream outfile2("save/ipbans/sid/" + registersid + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registergid != "" && registergid != "none") {
					std::ofstream outfile2("save/ipbans/gid/" + registergid + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registervid != "" && registervid != "none") {
					std::ofstream outfile2("save/ipbans/vid/" + registervid + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registeraid != "" && registeraid != "none") {
					std::ofstream outfile2("save/ipbans/aid/" + registeraid + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (registerIP != "" && registerIP != "none") {
					std::ofstream outfile2("save/ipbans/ip/" + registerIP + ".txt");
					outfile2 << "user who banned this ID: System" << endl;
					outfile2 << "Ban-ip reason: " << endl;
					outfile2 << "Banned user name is: " + nick;
					outfile2.close();
				}
				if (std::experimental::filesystem::exists("save/verify/" + registerIP + ".txt")) {
					string str = "save/verify/" + registerIP + ".txt";
					const char* c = str.c_str();
					remove(c);
				}
				ofstream write_player("save/players/_" + nick + ".json");
				write_player << j << std::endl;
				write_player.close();
			}
			catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
			string userdisplay = "";
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == nick) {
					userdisplay = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
				}
			} if (userdisplay == "") userdisplay = nick;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				//The Ancients
				Player::OnConsoleMessage(currentPeer, "`#** `oThe Ancients have used `#IP Ban `oon `w" + userdisplay + "`o! `#**");
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == nick) {
					Player::OnAddNotification(currentPeer, "`wWarning from `4System`w: You've been `4IP-BANNED", "audio/hub_open.wav", "interface/atomic_button.rttex");
					Player::OnConsoleMessage(currentPeer, "`wWarning from `4System`w: You've been `4IP-BANNED");
					enet_peer_disconnect_later(currentPeer, 0);
				}
			}
			LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, nick, "IP Ban");
			send_logs(LogSType::BAN, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oused IP ban on `4" + nick);
			send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5just IP-banned `4" + nick);
			SendPunishView(peer, nick);
		}
		else if (str == "/nick")
		{
			restore_player_name(world, peer);
		}
		else if (str == "/hidenick")
		{
			restore_player_name(world, peer);
		}
		else if (str.substr(0, 11) == "/givelevel ") {
			if (str.substr(11, cch.length() - 11 - 1) == "") return;
			string ban_info = str;
			size_t extra_space = ban_info.find("  ");
			if (extra_space != std::string::npos) {
				ban_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string ban_user;
			string ban_time;
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /givelevel <user> <level>");
				return;
			}
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_user = ban_info.substr(0, pos);
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /givelevel <user> <level>");
				return;
			}
			ban_time = ban_info;
			string playerName = ban_user;
			string howmuchgems = ban_time;
			bool contains_non_int2 = !std::regex_match(howmuchgems, std::regex("^[0-9]+$"));
			if (contains_non_int2 == true) {
				return;
			}
			int kiek_gems = atoi(howmuchgems.c_str());
			transform(playerName.begin(), playerName.end(), playerName.begin(), ::tolower);
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
						Player::OnConsoleMessage(currentPeer, "`oYour level has been changed to " + to_string(kiek_gems));
						Player::OnConsoleMessage(peer, "`oChanged " + static_cast<PlayerInfo*>(currentPeer->data)->displayName + " level to " + to_string(kiek_gems));
						static_cast<PlayerInfo*>(currentPeer->data)->level = kiek_gems;
						LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Set " + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " level to " + to_string(kiek_gems));
						send_logs(LogSType::GIVELEVEL, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oused `c/givelevel " + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " " + to_string(kiek_gems));
						send_logs_to_chat(static_cast<PlayerInfo*>(peer->data)->displayName + " `o(`1" + static_cast<PlayerInfo*>(peer->data)->rawName + "`o) `5just gave `2" + to_string(kiek_gems)+" `5level to `4"+ static_cast<PlayerInfo*>(currentPeer->data)->rawName);
						break;
					}
				}
			}
		}
		else if (str.substr(0, 12) == "/kickserver ")
		{
			if (str.substr(12, cch.length() - 12 - 1) == "") return;
			string ban_info = str;
			size_t extra_space = ban_info.find("  ");
			if (extra_space != std::string::npos) {
				ban_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string ban_user;
			string ban_time;
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_info.erase(0, pos + delimiter.length());
			}
			else {
				Player::OnConsoleMessage(peer, "`oUsage: /kickserver <user> <reason>");
				return;
			}
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_user = ban_info.substr(0, pos);
				ban_info.erase(0, pos + delimiter.length());
			}
			else {
				Player::OnConsoleMessage(peer, "`oUsage: /kickserver <user> <reason>");
				return;
			}
			ban_time = ban_info;
			string playerName = ban_user;
			string kickreason = ban_time;
			transform(playerName.begin(), playerName.end(), playerName.begin(), ::tolower);
			bool online = false;
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
						online = true;
						Player::OnConsoleMessage(currentPeer, "`4WARNING! `5You have been kicked from the server by `w" + static_cast<PlayerInfo*>(currentPeer->data)->displayName+ "`5. Reason: `1"+kickreason);
						Player::OnAddNotification(currentPeer, "`4WARNING! `5You have been kicked from the server by `w" + static_cast<PlayerInfo*>(currentPeer->data)->displayName + "`5. Reason: `1" + kickreason, "audio/already_used.wav", "interface/science_button.rttex");
						enet_peer_disconnect_later(currentPeer, 0);
						break;
					}
				}
			}
			if (!online)
			{
				Player::OnTextOverlay(peer, playerName + " `4is not online!");
			}
		}
		else if (str.substr(0, 10) == "/givegems ") {
			if (std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				sendWrongCmd(peer);
				return;
			}
			if (str.substr(10, cch.length() - 10 - 1) == "") return;
			string ban_info = str;
			size_t extra_space = ban_info.find("  ");
			if (extra_space != std::string::npos) {
				ban_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string ban_user;
			string ban_time;
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /givegems <user> <gems>");
				return;
			}
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_user = ban_info.substr(0, pos);
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /givegems <user> <gems>");
				return;
			}
			ban_time = ban_info;
			string playerName = ban_user;
			string howmuchgems = ban_time;
			bool contains_non_int2 = !std::regex_match(howmuchgems, std::regex("^[0-9]+$"));
			if (contains_non_int2 == true) {
				return;
			}
			int kiek_gems = atoi(howmuchgems.c_str());
			transform(playerName.begin(), playerName.end(), playerName.begin(), ::tolower);
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName) {
					if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
						Player::OnConsoleMessage(currentPeer, "`oYou received " + to_string(kiek_gems) + " gems");
						Player::OnConsoleMessage(peer, "`oSent " + to_string(kiek_gems) + " gems to " + static_cast<PlayerInfo*>(currentPeer->data)->displayName);
						std::ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						std::string content((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
						int gembux = atoi(content.c_str());
						int fingembux = gembux + kiek_gems;
						ofstream myfile;
						myfile.open("save/gemdb/_" + static_cast<PlayerInfo*>(currentPeer->data)->rawName + ".txt");
						myfile << fingembux;
						myfile.close();
						int gemcalc = gembux + kiek_gems;
						Player::OnSetBux(currentPeer, gemcalc, 0);
						LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Gave " + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " " + to_string(kiek_gems) + " gems");
						send_logs(LogSType::GIVEGEMS, "`4" + static_cast<PlayerInfo*>(peer->data)->rawName + " `oused `c/givegems " + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " " + to_string(kiek_gems));
						break;
					}
				}
			}
		}
		else if (str.substr(0, 11) == "/givetitle ") {
			string name = str.substr(11, cch.length() - 11 - 1);
			if (name.size() < 3 || name.size() > 16) return;
			toLowerCase(name);
			try {
				std::ifstream read_player("save/players/_" + name + ".json");
				if (!read_player.is_open()) {
					Player::OnConsoleMessage(peer, name + " does not exist!");
					return;
				}
				json j;
				read_player >> j;
				read_player.close();
				if (j["ltitleunlocked"]) {
					j["ltitleunlocked"] = false;
					j["ltitle"] = false;
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == name) {
							if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
								Player::OnConsoleMessage(currentPeer, "`oWarning from `4System`o: your legendary title has been `@disabled!");
								static_cast<PlayerInfo*>(currentPeer->data)->ltitle = false;
								static_cast<PlayerInfo*>(currentPeer->data)->ltitleunlocked = false;
								size_t pos;
								while ((pos = static_cast<PlayerInfo*>(currentPeer->data)->displayName.find(" of Legend``")) != string::npos) {
									static_cast<PlayerInfo*>(currentPeer->data)->displayName.replace(pos, 12, "");
								} if (isWorldOwner(peer, world)) {
									static_cast<PlayerInfo*>(currentPeer->data)->displayName = "`2" + static_cast<PlayerInfo*>(currentPeer->data)->displayName;
									Player::OnNameChanged(peer, static_cast<PlayerInfo*>(currentPeer->data)->netID, "`2" + static_cast<PlayerInfo*>(currentPeer->data)->displayName);
								} else {
									Player::OnNameChanged(peer, static_cast<PlayerInfo*>(currentPeer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->displayName);
								}
								LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Disabled legendary title for " + static_cast<PlayerInfo*>(currentPeer->data)->rawName);
								break;
							}
						}
					}
					Player::OnConsoleMessage(peer, name + " `onow has legendary title `@disabled`o!");
				} else {
					j["ltitleunlocked"] = true;
					j["ltitle"] = true;
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == name) {
							if (static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
								Player::OnConsoleMessage(currentPeer, "`oWarning from `4System`o: your legendary title has been `^enabled!");
								static_cast<PlayerInfo*>(currentPeer->data)->ltitle = true;
								static_cast<PlayerInfo*>(currentPeer->data)->ltitleunlocked = true;
								if (static_cast<PlayerInfo*>(currentPeer->data)->ltitle && static_cast<PlayerInfo*>(currentPeer->data)->ltitleunlocked && static_cast<PlayerInfo*>(currentPeer->data)->displayName.find(" of Legend``") == string::npos) {
									static_cast<PlayerInfo*>(currentPeer->data)->displayName += " of Legend``";
								} if (isWorldOwner(peer, world)) {
									static_cast<PlayerInfo*>(currentPeer->data)->displayName = "`2" + static_cast<PlayerInfo*>(currentPeer->data)->displayName;
									Player::OnNameChanged(peer, static_cast<PlayerInfo*>(currentPeer->data)->netID, "`2" + static_cast<PlayerInfo*>(currentPeer->data)->displayName);
								} else {
									Player::OnNameChanged(peer, static_cast<PlayerInfo*>(currentPeer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->displayName);
								}
								LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Enabled legendary title for " + static_cast<PlayerInfo*>(currentPeer->data)->rawName);
								break;
							}
						}
					}
					Player::OnConsoleMessage(peer, name + " `onow has legendary title `^enabled`o!");
				}
				ofstream write_player("save/players/_" + name + ".json");
				write_player << j << std::endl;
				write_player.close();
			} catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return; 
			}
		}
		else if (str.substr(0, 11) == "/givedoctor ") {
		string target_name = PlayerDB::getProperName(str.substr(11, cch.length() - 11 - 1));
		bool existxx = std::experimental::filesystem::exists("save/players/_" + PlayerDB::getProperName(target_name) + ".json");
		if (!existxx)
		{
			Player::OnTextOverlay(peer, "`4User doesn't exist!");
			return;
		}

		ifstream fg("save/players/_" + PlayerDB::getProperName(target_name) + ".json");
		json j;
		fg >> j;
		fg.close();

		j["drtitle"] = true;
		j["drtitleunlocked"] = true;

		ofstream fs("save/players/_" + PlayerDB::getProperName(target_name) + ".json");
		fs << j;
		fs.close();

		ENetPeer* currentPeer;
		for (currentPeer = server->peers;
			currentPeer < &server->peers[server->peerCount];
			++currentPeer)
		{
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
				continue;
			if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == PlayerDB::getProperName(target_name))
			{
				static_cast<PlayerInfo*>(currentPeer->data)->drtitle = true;
				static_cast<PlayerInfo*>(currentPeer->data)->drtitleunlocked = true;
				enet_peer_disconnect_later(currentPeer, 0);
				Player::OnConsoleMessage(currentPeer, "`4System-Message: `9Owner `2 " + static_cast<PlayerInfo*>(peer->data)->rawName + " `8has just gave you `8Doctor Title.");
			}
		}
		Player::OnConsoleMessage(peer, "`2You successfully gave `8Doctor Title to : `2" + target_name);
			}
		else if (str.substr(0, 10) == "/giverank ") {
			if (std::find(creators.begin(), creators.end(), static_cast<PlayerInfo*>(peer->data)->rawName) == creators.end()) {
				sendWrongCmd(peer);
				return;
			}
			if (str.substr(10, cch.length() - 10 - 1) == "") return;
			string ban_info = str;
			size_t extra_space = ban_info.find("  ");
			if (extra_space != std::string::npos) {
				ban_info.replace(extra_space, 2, " ");
			}
			string delimiter = " ";
			size_t pos = 0;
			string ban_user;
			string ban_time;
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /giverank <user> <rankname>");
				return;
			}
			if ((pos = ban_info.find(delimiter)) != std::string::npos) {
				ban_user = ban_info.substr(0, pos);
				ban_info.erase(0, pos + delimiter.length());
			} else {
				Player::OnConsoleMessage(peer, "`oUsage: /giverank <user> <rankname>");
				return;
			}
			ban_time = ban_info;
			string playerName = ban_user;
			string rankName = ban_time;
			bool success = false;
			transform(rankName.begin(), rankName.end(), rankName.begin(), ::tolower);
			transform(playerName.begin(), playerName.end(), playerName.begin(), ::tolower);
			string userdisplay = "";
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName) {
					userdisplay = static_cast<PlayerInfo*>(currentPeer->data)->displayName;
				}
			} if (userdisplay == "") userdisplay = playerName;
			GiveRank(rankName, playerName, success);
			if (success) {
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->rawName == playerName && static_cast<PlayerInfo*>(currentPeer->data)->haveGrowId) {
						Player::OnConsoleMessage(currentPeer, "`oWarning from `4System`o: your rank has been `5Changed to `8" + rankName);
						enet_peer_disconnect_later(currentPeer, 0);
						Player::OnConsoleMessage(peer, "`2Successfully changed.");
						LogAccountActivity(static_cast<PlayerInfo*>(peer->data)->rawName, static_cast<PlayerInfo*>(peer->data)->rawName, "Changed " + static_cast<PlayerInfo*>(currentPeer->data)->rawName + " role to " + rankName + "");
						send_logs(LogSType::GIVERANK, "`4"+static_cast<PlayerInfo*>(peer->data)->rawName+" `ochanged rank for player `4"+static_cast<PlayerInfo*>(currentPeer->data)->rawName + "`o to `4"+rankName+" `orank.");
					}
					Player::OnConsoleMessage(currentPeer, "`#** `$" + (role_prefix.at(static_cast<PlayerInfo*>(peer->data)->adminLevel) + static_cast<PlayerInfo*>(peer->data)->rawName) + " `ogave " + rankName + " `orank to `w" + userdisplay + "`o! `#**");
				}
			} else {
				Player::OnConsoleMessage(peer, "`4An error occurred. `2It could be because you entered the wrong player name or rank name.");
			}
		}
		else if (str.substr(0, 6) == "/nick ") {
			string name2 = str.substr(6, cch.length() - 6 - 1);
			if ((str.substr(6, cch.length() - 6 - 1).find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ") != string::npos)) return;
			if (name2.length() < 3) return;
			if (name2.length() > 16) return;
			if (name2.find(" of Legend") != string::npos) {
				return;
			}
			string backup_name = name2;
			size_t pos;
			while ((pos = backup_name.find("`")) != string::npos) {
				backup_name.replace(pos, 1, "");
			}
			if (backup_name.size() < 3) return;
			static_cast<PlayerInfo*>(peer->data)->msgName = PlayerDB::getProperName(str.substr(6, cch.length() - 6 - 1));
			static_cast<PlayerInfo*>(peer->data)->OriName = name2;
			if (static_cast<PlayerInfo*>(peer->data)->NickPrefix != "")
			{
				static_cast<PlayerInfo*>(peer->data)->displayName = static_cast<PlayerInfo*>(peer->data)->NickPrefix + ". " + str.substr(6, cch.length() - 6 - 1);
				name2 = static_cast<PlayerInfo*>(peer->data)->NickPrefix + ". " + name2;
			}
			else static_cast<PlayerInfo*>(peer->data)->displayName = str.substr(6, cch.length() - 6 - 1);
			if (static_cast<PlayerInfo*>(peer->data)->ltitle && static_cast<PlayerInfo*>(peer->data)->ltitleunlocked && static_cast<PlayerInfo*>(peer->data)->displayName.find(" of Legend``") == string::npos) {
				static_cast<PlayerInfo*>(peer->data)->displayName += " of Legend``";
			}
			static_cast<PlayerInfo*>(peer->data)->isNicked = true;
			if (isWorldOwner(peer, world)) {
				//if (static_cast<PlayerInfo*>(peer->data)->displayName.find("`") != string::npos) {} else {
					static_cast<PlayerInfo*>(peer->data)->displayName = "`2" + static_cast<PlayerInfo*>(peer->data)->displayName;
					Player::OnNameChanged(peer, static_cast<PlayerInfo*>(peer->data)->netID, "`2" + static_cast<PlayerInfo*>(peer->data)->displayName);
				//}
			} else {
				Player::OnNameChanged(peer, static_cast<PlayerInfo*>(peer->data)->netID, static_cast<PlayerInfo*>(peer->data)->displayName);
			}
		}
		else if (str == "/magic") {
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
				if (isHere(peer, currentPeer)) {
					Player::PlayAudio(currentPeer, "audio/magic.wav", 0);
					for (int i = 0; i < 14; i++) {
						if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
						if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
						if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
						if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);

						if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
						if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
						if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
						if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
					}
				}
			}
		}
		else if (str == "/invis" || str == "/inviS" || str == "/INVIS" || str == "/Invis" || str == "/inVis") {
		if (static_cast<PlayerInfo*>(peer->data)->isinv == false) {
			if (world->name == "GAME1") {
				Player::OnConsoleMessage(peer, "Burada bu komutu kullanamazsin.");
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->job != "")
			{
				Player::OnConsoleMessage(peer, "Suanda bir isin var gorunmeze gecemezsin.");
				return;
			}
			static_cast<PlayerInfo*>(peer->data)->isinv = true;
			Player::OnConsoleMessage(peer, "`oArtik gorunmezsin, hemde herkese.");
			Player::PlayAudio(peer, "audio/boo_ghost_be_gone.wav", 0);
				gamepacket_t p(0, static_cast<PlayerInfo*>(peer->data)->netID);
				p.Insert("OnInvis");
				p.Insert(1);
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						p.CreatePacket(currentPeer);
						Player::PlayAudio(currentPeer, "audio/magic.wav", 0);
						for (int i = 0; i < 14; i++) {
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);

							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
						}
					}
				}
			} else {
			Player::OnConsoleMessage(peer, "Artik gorunmez degilsin herkes seni gorebilir atomlarindan ayrildin.");
			static_cast<PlayerInfo*>(peer->data)->isinv = false;
				Player::PlayAudio(peer, "audio/boo_proton_glove.wav", 0);
				gamepacket_t p(0, static_cast<PlayerInfo*>(peer->data)->netID);
				p.Insert("OnInvis");
				p.Insert(0);
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						p.CreatePacket(currentPeer);
						Player::PlayAudio(currentPeer, "audio/magic.wav", 0);
						for (int i = 0; i < 14; i++) {
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);
							if (rand() % 100 <= 75) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 6 + 1, 2, i * 300);

							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y - 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x + 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
							if (rand() % 100 <= 25) SendParticleEffect(currentPeer, static_cast<PlayerInfo*>(peer->data)->x - 15 * (rand() % 6), static_cast<PlayerInfo*>(peer->data)->y + 15 * (rand() % 6), rand() % 16, 3, i * 300);
					}
				}
			}
		}
		}
		else if (str.substr(0, 5) == "/jsb ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			string sb_text = str.substr(5, cch.length() - 5 - 1);
			string name = static_cast<PlayerInfo*>(peer->data)->displayName;
			Player::OnConsoleMessage(peer, "`2>> `9Jammed Broadcast sent to all players online`2!");
			GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[SB]_ `#** `#from (`2" + name + "`#) in [`4JAMMED!`#] ** : `o" + sb_text));
			string text = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0\n";
			BYTE* data = new BYTE[5 + text.length()];
			BYTE zero = 0;
			int type = 3;
			memcpy(data, &type, 4);
			memcpy(data + 4, text.c_str(), text.length());
			memcpy(data + 4 + text.length(), &zero, 1);
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (!static_cast<PlayerInfo*>(currentPeer->data)->radio)
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT")
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->isIn == false)
					continue;
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet);
				ENetPacket* packet2 = enet_packet_create(data,
					5 + text.length(),
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet2);
				//enet_host_flush(server);
			}
			delete[] data;
			delete p.data;
		}
		else if (str.substr(0, 5) == "/csb ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			string sb_text = str.substr(5, cch.length() - 5 - 1);
			string name = static_cast<PlayerInfo*>(peer->data)->displayName;
			Player::OnConsoleMessage(peer, "`2>> `9Crows Broadcast sent to all players online`2!");
			GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[SB]_ `5** `5from (`2" + name + "`5) in `5[`4<HIDDEN>`5] ** : `5" + sb_text));
			string text = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0\n";
			BYTE* data = new BYTE[5 + text.length()];
			BYTE zero = 0;
			int type = 3;
			memcpy(data, &type, 4);
			memcpy(data + 4, text.c_str(), text.length());
			memcpy(data + 4 + text.length(), &zero, 1);
			ENetPeer* currentPeer;
			for (currentPeer = server->peers;
				currentPeer < &server->peers[server->peerCount];
				++currentPeer)
			{
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
					continue;
				if (!static_cast<PlayerInfo*>(currentPeer->data)->radio)
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT")
					continue;
				if (static_cast<PlayerInfo*>(currentPeer->data)->isIn == false)
					continue;
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet);
				ENetPacket* packet2 = enet_packet_create(data,
					5 + text.length(),
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(currentPeer, 0, packet2);
				//enet_host_flush(server);
			}
			delete[] data;
			delete p.data;
		}
		else if (str.substr(0, 4) == "/sb ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				Player::OnConsoleMessage(peer, "`@Super Broadcast Not `4Allowed `@When You Are `9Duct-taped`@!");
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->level < 5 && !static_cast<PlayerInfo*>(peer->data)->Subscriber)
			{
				Player::OnConsoleMessage(peer, ">> `4OOPS:`` To cut down on `4spam`` the broadcast features are only available to who are level `55`` and higher!");
				return;
			}
			/*ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
			string content((std::istreambuf_iterator<char>(ifsz)), (std::istreambuf_iterator<char>()));
			int b = atoi(content.c_str());
			if (b > 1000)
			{*/
				if (static_cast<PlayerInfo*>(peer->data)->lastSB + 60000 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
				{
					static_cast<PlayerInfo*>(peer->data)->lastSB = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
				}
				else
				{
					int kiekDar = (static_cast<PlayerInfo*>(peer->data)->lastSB + 60000 - (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count()) / 1000;
					Player::OnConsoleMessage(peer, "`9Cooldown `@Please Wait `9" + to_string(kiekDar) + " Seconds `@To Throw Another Broadcast!");
					return;
				}
				/*int gemcalc10k = b - 1000;
				ofstream myfile2;
				myfile2.open("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
				myfile2 << std::to_string(gemcalc10k);
				myfile2.close();
				ifstream ifszi("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
				string contentx((std::istreambuf_iterator<char>(ifszi)), (std::istreambuf_iterator<char>()));
				int updgem = atoi(contentx.c_str());
				GamePacket pp = packetEnd(appendInt(appendString(createPacket(), "OnSetBux"), updgem));
				ENetPacket* packetpp = enet_packet_create(pp.data, pp.len, ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packetpp);
				delete pp.data;*/
				string sb_text = str.substr(4, cch.length() - 4 - 1);
				Player::OnConsoleMessage(peer, "`o>> Super Broadcast sent to all players online!");
				string worldname = static_cast<PlayerInfo*>(peer->data)->currentWorld;
				if (jammers) {
					for (auto i = 0; i < world->width * world->height; i++) {
						if (world->items.at(i).foreground == 226 && world->items.at(i).activated) {
							worldname = "`4JAMMED!";
							break;
						}
					}
				}
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "CP:_PL:0_OID:_CT:[SB]_ `5** `5from (`2" + name + "`5) in [`o" + worldname + "`5] ** :`$ " + sb_text));
				string text = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0\n";
				BYTE* data = new BYTE[5 + text.length()];
				BYTE zero = 0;
				int type = 3;
				memcpy(data, &type, 4);
				memcpy(data + 4, text.c_str(), text.length());
				memcpy(data + 4 + text.length(), &zero, 1);
				ENetPeer* currentPeer;
				for (currentPeer = server->peers;
					currentPeer < &server->peers[server->peerCount];
					++currentPeer)
				{
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (!static_cast<PlayerInfo*>(currentPeer->data)->radio) continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->isIn == false) continue;
					ENetPacket* packet = enet_packet_create(p.data, p.len, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet);
					ENetPacket* packet2 = enet_packet_create(data, 5 + text.length(), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(currentPeer, 0, packet2);
					static_cast<PlayerInfo*>(currentPeer->data)->lastSbbWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
				}
				delete[] data;
				delete p.data;
			/*}
			else
			{
				int needgems = 1000 - b;
				Player::OnConsoleMessage(peer, "`@You Need `9" + to_string(needgems) + " `@Gems More To Send Super Broadcast!");
			}*/
		}
		else if (str.substr(0, 7) == "/schat ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`@Staff Chat Not `4Allowed `@When You Are `9Duct-taped`@!"));
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete p.data;
			}
			else
			{
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`w** `5[STAFF-CHAT]`` from `$`2" + name + "`$: `# " + str.substr(7, cch.length() - 7 - 1)));
				string text = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0\n";
				BYTE* data = new BYTE[5 + text.length()];
				BYTE zero = 0;
				int type = 3;
				memcpy(data, &type, 4);
				memcpy(data + 4, text.c_str(), text.length());
				memcpy(data + 4 + text.length(), &zero, 1);
				ENetPeer* currentPeer;
				for (currentPeer = server->peers;
					currentPeer < &server->peers[server->peerCount];
					++currentPeer)
				{
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
						continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT")
						continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->isIn == false)
						continue;
					if (isMod(currentPeer))
					{
						ENetPacket* packet = enet_packet_create(p.data,
							p.len,
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(currentPeer, 0, packet);
						ENetPacket* packet2 = enet_packet_create(data,
							5 + text.length(),
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(currentPeer, 0, packet2);
					}
				}
				delete[] data;
				delete p.data;
			}
		}
		else if (str.substr(0, 5) == "/sdb ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`@Super Duper Broadcast Not `4Allowed `@When You Are `9Duct-taped`@!"));
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete p.data;
			}
			else
			{
				if (static_cast<PlayerInfo*>(peer->data)->level < 60)
				{
					GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`@You Must Be Aleast `9Level `460 `@To Use This `9Command`@!"));
					ENetPacket* packet = enet_packet_create(p.data,
						p.len,
						ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(peer, 0, packet);
					delete p.data;
				}
				else
				{
					std::ifstream ifsz("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
					std::string content((std::istreambuf_iterator<char>(ifsz)),
						(std::istreambuf_iterator<char>()));
					int b = atoi(content.c_str());
					if (b > 100000)
					{
						if (static_cast<PlayerInfo*>(peer->data)->lastSDB + 600000 < (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count())
						{
							static_cast<PlayerInfo*>(peer->data)->lastSDB = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
						}
						else
						{
							GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`9Cooldown `@Please Wait `910 minutes `@To Throw Another Super-Duper-Broadcast!"));
							ENetPacket* packet = enet_packet_create(p.data,
								p.len,
								ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(peer, 0, packet);
							delete p.data;
							//enet_host_flush(server);
							return;
						}
						int gemcalc10k = b - 100000;
						ENetPeer* currentPeer;
						for (currentPeer = server->peers;
							currentPeer < &server->peers[server->peerCount];
							++currentPeer)
						{
							if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
								continue;
							static_cast<PlayerInfo*>(currentPeer->data)->lastSdbWorld = static_cast<PlayerInfo*>(peer->data)->currentWorld;
						}
						ofstream myfile2;
						myfile2.open("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
						myfile2 << std::to_string(gemcalc10k);
						myfile2.close();
						std::ifstream ifszi("save/gemdb/_" + static_cast<PlayerInfo*>(peer->data)->rawName + ".txt");
						std::string contentx((std::istreambuf_iterator<char>(ifszi)),
							(std::istreambuf_iterator<char>()));
						int updgem = atoi(contentx.c_str());
						GamePacket pp = packetEnd(appendInt(appendString(createPacket(), "OnSetBux"), updgem));
						ENetPacket* packetpp = enet_packet_create(pp.data,
							pp.len,
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(peer, 0, packetpp);
						delete pp.data;
						GamePacket p5 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`2>> `9Super Duper Broadcast sent to all players online`2!"));
						ENetPacket* packet5 = enet_packet_create(p5.data,
							p5.len,
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(peer, 0, packet5);
						delete p5.data;
						string name = static_cast<PlayerInfo*>(peer->data)->displayName;
						GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnDialogRequest"), "set_default_color|`o\n\nadd_label_with_icon|big|`^Super Duper Broadcast`^!``|left|2480|\n\nadd_spacer|small|\nadd_label|small|`#From " + name + "|left|4|\nadd_label|small|`2>> `@" + str.substr(4, cch.length() - 4 - 1) + "|\n\nadd_spacer|small|\nadd_button|warptosb|`2Warp To `9" + static_cast<PlayerInfo*>(peer->data)->currentWorld + "`2!|\nadd_quick_exit|\n"));
						string text = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0\n";
						BYTE* data = new BYTE[5 + text.length()];
						BYTE zero = 0;
						int type = 3;
						memcpy(data, &type, 4);
						memcpy(data + 4, text.c_str(), text.length());
						memcpy(data + 4 + text.length(), &zero, 1);

						ENetPeer* currentPeer0;
						for (currentPeer0 = server->peers;
							currentPeer0 < &server->peers[server->peerCount];
							++currentPeer0)
						{
							if (currentPeer0->state != ENET_PEER_STATE_CONNECTED)
								continue;
							if (!static_cast<PlayerInfo*>(currentPeer0->data)->radio)
								continue;
							if (static_cast<PlayerInfo*>(currentPeer0->data)->currentWorld == "EXIT")
								continue;
							if (static_cast<PlayerInfo*>(currentPeer0->data)->isIn == false)
								continue;
							ENetPacket* packet = enet_packet_create(p.data,
								p.len,
								ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(currentPeer0, 0, packet);
							ENetPacket* packet2 = enet_packet_create(data,
								5 + text.length(),
								ENET_PACKET_FLAG_RELIABLE);
							enet_peer_send(currentPeer0, 0, packet2);
							//enet_host_flush(server);
						}
						delete[] data;
						delete p.data;
					}
					else
					{
						int needgems = 100000 - b;
						GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`@You Need `9" + to_string(needgems) + " `@Gems More To Send Super Duper Broadcast!"));
						ENetPacket* packet = enet_packet_create(p.data,
							p.len,
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(peer, 0, packet);
						delete p.data;
					}
				}
			}
		}
		else if (str.substr(0, 3) == "/g ")
		{
			if (static_cast<PlayerInfo*>(peer->data)->isDuctaped == true)
			{
				GamePacket p0 = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`4You are muted now!"));
				ENetPacket* packet0 = enet_packet_create(p0.data,
					p0.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet0);
				delete p0.data;
				return;
			}
			if (static_cast<PlayerInfo*>(peer->data)->guild != "")
			{
				string name = static_cast<PlayerInfo*>(peer->data)->displayName;
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`w[`5GUILD CHAT`w] [`4" + static_cast<PlayerInfo*>(peer->data)->tankIDName + "`w]  = " + str.substr(3, cch.length() - 3 - 1)));
				ENetPeer* currentPeer;
				for (currentPeer = server->peers;
					currentPeer < &server->peers[server->peerCount];
					++currentPeer)
				{
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL)
						continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == "EXIT")
						continue;
					if (static_cast<PlayerInfo*>(currentPeer->data)->isIn == false)
						continue;
					if (find(static_cast<PlayerInfo*>(peer->data)->guildMembers.begin(), static_cast<PlayerInfo*>(peer->data)->guildMembers.end(), static_cast<PlayerInfo*>(currentPeer->data)->rawName) != static_cast<PlayerInfo*>(peer->data)->guildMembers.end())
					{
						ENetPacket* packet = enet_packet_create(p.data,
							p.len,
							ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(currentPeer, 0, packet);
					}
				}
				delete p.data;
			}
			else
			{
				Player::OnConsoleMessage(peer, "You won't see broadcasts anymore.");
				GamePacket p = packetEnd(appendString(appendString(createPacket(), "OnConsoleMessage"), "`9Sorry! `^You must join a `9Guild `^Or `9Create `^One to use this command!"));
				ENetPacket* packet = enet_packet_create(p.data,
					p.len,
					ENET_PACKET_FLAG_RELIABLE);
				enet_peer_send(peer, 0, packet);
				delete p.data;
				return;
			}
		} 
		else if (str.substr(0, 6) == "/radio") {
			if (static_cast<PlayerInfo*>(peer->data)->radio) {
				Player::OnConsoleMessage(peer, "You won't see broadcasts anymore.");
				static_cast<PlayerInfo*>(peer->data)->radio = false;
			} else {
				Player::OnConsoleMessage(peer, "You will now see broadcasts again.");
				static_cast<PlayerInfo*>(peer->data)->radio = true;
			}
		} 
		else if (str.substr(0, 7) == "/color ") {
			if (str.substr(7, cch.length() - 7 - 1).size() >= 20 || str.substr(7, cch.length() - 7 - 1).size() <= 0) return;
			int color = atoi(str.substr(7, cch.length() - 7 - 1).c_str());
			static_cast<PlayerInfo*>(peer->data)->skinColor = color;
			sendClothes(peer);
		} 
		else if (str.substr(0, 4) == "/who") {
			sendWho(peer);
			}
		else if (str == "/cry") {
		GamePacket p2 = packetEnd(appendIntx(appendString(appendIntx(appendString(createPacket(), "OnTalkBubble"), ((PlayerInfo*)(peer->data))->netID), ":'("), 0));
		ENetPacket* packet2 = enet_packet_create(p2.data,
			p2.len,
			ENET_PACKET_FLAG_RELIABLE);
		ENetPeer* currentPeer;
		for (currentPeer = server->peers;
			currentPeer < &server->peers[server->peerCount];
			++currentPeer)
		{
			if (currentPeer->state != ENET_PEER_STATE_CONNECTED)
				continue;
			if (isHere(peer, currentPeer)) {
				enet_peer_send(currentPeer, 0, packet2);
			}
		}
		delete p2.data;
						}
		else if (str.rfind("/", 0) == 0 && str != "/cheer" && str != "/dance" && str != "/cry" && str != "/troll" && str != "/sleep" && str != "/dance2" && str != "/love" && str != "/dab" && str != "/wave" && str != "/furious" && str != "/fp" && str != "/yes" && str != "/no" && str != "/omg" && str != "/idk" && str != "/rolleyes" && str != "/fold" && str != "/sassy") {
			sendWrongCmd(peer);
		} else {
			if (str.rfind("/", 0) == 0) return;
			message = trimString(message);

			if (static_cast<PlayerInfo*>(peer->data)->messageLetters >= 500)
			{
				static_cast<PlayerInfo*>(peer->data)->messagecooldown = true;
				static_cast<PlayerInfo*>(peer->data)->messagecooldowntime = GetCurrentTimeInternalSeconds() + 25;
				Player::OnConsoleMessage(peer, "`6>>`4Spam Decected! `6Please wait a bit before typing anything else. Please note, any form of bot/macro/auto-paste will get all your accounts banned so don't do it!");
				return;
			}

			string check_msg = message;
			toUpperCase(check_msg);
			if (check_msg == ":)" || check_msg == ":(" || check_msg == ":*" || check_msg == ":'(" || check_msg == ":D" || check_msg == ":O" || check_msg == ";)" || check_msg == ":O.O" || check_msg == ":p")  {
				for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (isHere(peer, currentPeer)) {
						Player::OnTalkBubble(currentPeer, static_cast<PlayerInfo*>(peer->data)->netID, check_msg, 0, false);
					}
				}
				return;
			}
			string ccode = "", chatcode = "";
			ccode = role_chat.at(static_cast<PlayerInfo*>(peer->data)->adminLevel);
			chatcode = role_chat.at(static_cast<PlayerInfo*>(peer->data)->adminLevel);
			if (ccode == "w") chatcode = "$";
			if (static_cast<PlayerInfo*>(peer->data)->isNicked && isMod(peer)) {
				ccode = "w";
				chatcode = "$";
			}  if (static_cast<PlayerInfo*>(peer->data)->Subscriber && static_cast<PlayerInfo*>(peer->data)->chatcolor != "" || isDev(peer) && static_cast<PlayerInfo*>(peer->data)->chatcolor != "") {
				ccode = static_cast<PlayerInfo*>(peer->data)->chatcolor;
				chatcode = static_cast<PlayerInfo*>(peer->data)->chatcolor;
			} 
			if (allCharactersSame(message)) return;
			for (auto c : message) {
				if (c < 0x18 || std::all_of(message.begin(), message.end(), static_cast<int(*)(int)>(isspace))) return;
			} 
			//if (webhooks) threads.push_back(std::thread(SendWebhook, static_cast<PlayerInfo*>(peer->data), message));
			for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
				if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
				if (isHere(peer, currentPeer)) {
					Player::OnConsoleMessage(currentPeer, "CP:_PL:0_OID:_CT:[W]_ `6<`w" + static_cast<PlayerInfo*>(peer->data)->displayName + "`6> `" + chatcode + message);
					Player::OnTalkBubble(currentPeer, static_cast<PlayerInfo*>(peer->data)->netID, "`" + ccode + message, 0, false);
				}
			}
			if (static_cast<PlayerInfo*>(peer->data)->antispamlastmessage == "")
			{
				static_cast<PlayerInfo*>(peer->data)->antispamlastmessage = message;
			}
			else if(static_cast<PlayerInfo*>(peer->data)->antispamlastmessage == message)
			{
				static_cast<PlayerInfo*>(peer->data)->antispamlastmessagecount++;
				if (static_cast<PlayerInfo*>(peer->data)->antispamlastmessagecount >= 3)
				{
					for (ENetPeer* currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {

						if (!static_cast<PlayerInfo*>(peer->data)->isNicked && !isMod(peer)) {
							static_cast<PlayerInfo*>(peer->data)->messagecooldown = true;
							static_cast<PlayerInfo*>(peer->data)->messagecooldowntime = GetCurrentTimeInternalSeconds() + 25;
							Player::OnConsoleMessage(peer, "2`6>>`4Spam Decected! `6Please wait a bit before typing anything else. Please note, any form of bot/macro/auto-paste will get all your accounts banned so don't do it!");
							return;
						}
					}
				}
			}
			else
			{
				static_cast<PlayerInfo*>(peer->data)->antispamlastmessagecount = 0;
				static_cast<PlayerInfo*>(peer->data)->antispamlastmessage = message;
			}
			static_cast<PlayerInfo*>(peer->data)->messageLetters = static_cast<PlayerInfo*>(peer->data)->messageLetters + message.size();
		}
	} catch(const std::out_of_range& e) {
		std::cout << e.what() << std::endl;
	} 
}
