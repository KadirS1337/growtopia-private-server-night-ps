#pragma once

/*Dropping the event item*/
inline void SendDropEventItem(const string WorldName, const int netID, const int x, int y, const int item, const int count, const BYTE specialEffect)
{
	if (serverIsFrozen) return;
	if (item >= maxItems) return;
	if (item < 0) return;
	auto world = worldDB.get2(WorldName).ptr;
	DroppedItem itemDropped{};
	itemDropped.id = item;
	itemDropped.count = count;
	itemDropped.x = x;
	itemDropped.y = y;
	itemDropped.uid = world->droppedCount++;
	world->droppedItems.push_back(itemDropped);
	for (auto currentPeer = server->peers;
	     currentPeer < &server->peers[server->peerCount];
	     ++currentPeer)
	{
		if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
		if (EventWorld(currentPeer, WorldName))
		{
			PlayerMoving data{};
			data.packetType = 14;
			data.x = x;
			data.y = y;
			data.netID = netID;
			data.plantingTree = item;
			float val = count;
			auto val2 = specialEffect;
			const auto raw = packPlayerMoving(&data);
			memcpy(raw + 16, &val, 4);
			memcpy(raw + 1, &val2, 1);
			SendPacketRaw(4, raw, 56, nullptr, currentPeer, ENET_PACKET_FLAG_RELIABLE);
		}
	}
}

/*Spawning the event items*/
inline void SpawnEventItem(const string WorldName, const int ItemID, const int count, int Time, const int x, const int y, const int Multiple)
{
	if (serverIsFrozen) return;
	for (auto i = 0; i < Multiple; i++) {
		const auto DropX = rand() % x * 32;
		const auto DropY = rand() % y * 32;
		SendDropEventItem(WorldName, -1, DropX, DropY, ItemID, count, 0);
	}
}

/*Basically the all world event things, this is a loop which finds active worlds*/
inline void WorldEvents() {
	try {
		vector<WorldInfo*> worlds = getRandomWorldsPointer(worldthreading, 1);
		for (auto iss = 0; iss < worlds.size(); iss++) {
			if (loop_freeze) break;
			//if (find(offloaded_worlds.begin(), offloaded_worlds.end(), iss) != offloaded_worlds.end()) break;
			if (iss >= worlds.size()) {
				SendConsole("worlds.size() was larger than loop size (c63)", "ERROR");
				continue;
			}
			if (worlds.at(iss)->name == "error" || worlds.at(iss)->name == "EXIT") continue;
			//if (getPlayersCountInWorldEvent(worlds.at(iss), worlds.at(iss)->name) <= 0) continue;
			for (auto jas = 0; jas < worlds.at(iss)->width * worlds.at(iss)->height; jas++) {
				auto x = (jas % worlds.at(iss)->width);
				auto y = (jas / worlds.at(iss)->width);
				if (x < 0 || y < 0 || x > worlds.at(iss)->width - 1 || y > worlds.at(iss)->height - 1) continue;
				if (x / 32 >= 0 && y / 32 >= 0 && x / 32 <= worlds.at(iss)->width && y / 32 <= worlds.at(iss)->height) {
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 10) {
						if (rand() % 100 <= 3) {
							if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).evolvestage >= 30) {
								worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).evolvestage = 0;
								int RedBlocks = 0;
								int GreenBlocks = 0;
								int GoldenBlocks = 0;
								int BlueBlocks = 0;
								if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 4 || x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 4 || y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 4 || y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 4) {
									if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 4) RedBlocks++;
									if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 4) RedBlocks++;
									if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 4) RedBlocks++;
									if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 4) RedBlocks++;
								}
								if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 2 || x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 2 || y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 2 || y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 2) {
									if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 2) GreenBlocks++;
									if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 2) GreenBlocks++;
									if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 2) GreenBlocks++;
									if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 2) GreenBlocks++;
								}
								if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 260 || x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 260 || y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 260 || y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 260) {
									if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 260) GoldenBlocks++;
									if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 260) GoldenBlocks++;
									if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 260) GoldenBlocks++;
									if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 260) GoldenBlocks++;
								}
								if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).water || x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).water || y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).water || y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).water) {
									if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).water) BlueBlocks++;
									if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).water) BlueBlocks++;
									if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).water) BlueBlocks++;
									if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).water) BlueBlocks++;
								}
								int SpawnID = 0;
								if (RedBlocks >= GreenBlocks && RedBlocks >= GoldenBlocks && RedBlocks >= BlueBlocks) {
									SpawnID = 5174;
								}
								if (GreenBlocks >= RedBlocks && GreenBlocks >= GoldenBlocks && GreenBlocks >= BlueBlocks) {
									SpawnID = 5172;
								}
								if (GoldenBlocks >= GreenBlocks && GoldenBlocks >= RedBlocks && GoldenBlocks >= BlueBlocks) {
									SpawnID = 5160;
								}
								if (BlueBlocks >= GreenBlocks && BlueBlocks >= RedBlocks && BlueBlocks >= GoldenBlocks) {
									SpawnID = 5176;
								}
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
								data3.plantingTree = SpawnID;
								for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
									if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
									if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
										SendParticleEffect(currentPeer6, x * 32 + 16, y * 32 + 16, 4762, 97, 0);
										auto raw = packPlayerMoving(&data3);
										raw[2] = 0;
										raw[3] = 0;
										SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
									}
								}
								worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground = SpawnID;
							}
							else {
								worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).evolvestage++;
							}
						}
						//jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).fire) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 3528 && !worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).water && !worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).fire && !isSeed(worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground) && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 8 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6 && getItemDef(worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::LOCK && getItemDef(worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::DISPLAY && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6952 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6954 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 5638 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6946 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6948 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 2978 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 1420 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 6214 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 1006 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 656 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground != 1420 && getItemDef(worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::DONATION && Chance >= 75) {
							if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).background == 0 || worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).background == 6864) continue;
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).fire = true;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									WorldInfo* world = getPlyersWorld(currentPeer6);
									if (world == NULL) {
										SendConsole("ciklas error", "ERROR");
										return;
									}
									UpdateVisualsForBlock(currentPeer6, true, x + 1, y, world);
									Player::OnParticleEffect(currentPeer6, 150, x * 32, y * 32, 0);
								}
							}
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 3528 && !worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).water && !worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).fire && !isSeed(worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground) && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 8 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6 && getItemDef(worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::LOCK && getItemDef(worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::DISPLAY && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6952 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6954 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 5638 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6946 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6948 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 2978 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 1420 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 6214 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 1006 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 656 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground != 1420 && getItemDef(worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground).blockType != BlockTypes::DONATION && Chance >= 43) {
							if (worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).background == 0 || worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).background == 6864) continue;
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).fire = true;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									WorldInfo* world = getPlyersWorld(currentPeer6);
									if (world == NULL) {
										SendConsole("ciklas error", "ERROR");
										return;
									}
									UpdateVisualsForBlock(currentPeer6, true, x - 1, y, world);
									Player::OnParticleEffect(currentPeer6, 150, x * 32, y * 32, 0);
								}
							}
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 3528 && !worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).water && !worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).fire && !isSeed(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground) && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 8 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6 && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground).blockType != BlockTypes::LOCK && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground).blockType != BlockTypes::DISPLAY && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6952 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6954 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 5638 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6946 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6948 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 2978 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 1420 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 6214 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 1006 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 656 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground != 1420 && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground).blockType != BlockTypes::DONATION && Chance >= 20) {
							if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).background == 0 || worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).background == 6864) continue;
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).fire = true;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									WorldInfo* world = getPlyersWorld(currentPeer6);
									if (world == NULL) {
										SendConsole("ciklas error", "ERROR");
										return;
									}
									UpdateVisualsForBlock(currentPeer6, true, x, y - 1, world);
									Player::OnParticleEffect(currentPeer6, 150, x * 32, y * 32, 0);
								}
							}
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 3528 && !worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).water && !worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).fire && !isSeed(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground) && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 8 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6 && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground).blockType != BlockTypes::LOCK && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground).blockType != BlockTypes::DISPLAY && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6952 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6954 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 5638 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6946 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6948 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 2978 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 1420 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 6214 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 1006 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 656 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground != 1420 && getItemDef(worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground).blockType != BlockTypes::DONATION && Chance >= 0) {
							if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).background == 0 || worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).background == 6864) continue;
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).fire = true;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									WorldInfo* world = getPlyersWorld(currentPeer6);
									if (world == NULL) {
										SendConsole("ciklas error", "ERROR");
										return;
									}
									UpdateVisualsForBlock(currentPeer6, true, x, y + 1, world);
									Player::OnParticleEffect(currentPeer6, 150, x * 32, y * 32, 0);
								}
							}
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 4762 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 75) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 4762;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 4762, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 4762;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 43) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 4762;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 4762, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 4762;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 20) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 4762;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 4762, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 4762;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 0) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 4762;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 4762, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 4762;
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 7382 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 75) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 7382;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 7382, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 7382;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 43) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 7382;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 7382, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 7382;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 25) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 7382;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 7382, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 7382;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 5) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 7382;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 7382, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 7382;
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 5136 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 75) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5136;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 5136, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 5136;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 43) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5136;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 5136, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 5136;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 30) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5136;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5136, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 5136;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 10) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5136;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5136, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 5136;
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 5138 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 85) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5138;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 5138, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 5138;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 53) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5138;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 5138, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 5138;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 35) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5138;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5138, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 5138;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 15) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5138;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5138, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 5138;
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 5140 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 75) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5140;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 5140, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 5140;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 53) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5140;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 5140, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 5140;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 33) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5140;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5140, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 5140;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 20) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5140;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5140, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 5140;
						}
						jas += 1;
					}
					if (worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width)).foreground == 5154 && worlds.at(iss)->height == 60 && worlds.at(iss)->width == 90) {
						auto Chance = rand() % 100;
						if (worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 70) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x + 1;
							data3.y = y;
							data3.punchX = x + 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5154;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 + 45, y * 32 + 16, 5154, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + 1 + (y * worlds.at(iss)->width)).foreground = 5154;
						}
						else if (x > 0 && worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground == 0 && Chance >= 53) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x - 1;
							data3.y = y;
							data3.punchX = x - 1;
							data3.punchY = y;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5154;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32 - 22, y * 32 + 16, 5154, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x - 1 + (y * worlds.at(iss)->width)).foreground = 5154;
						}
						else if (y > 0 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground == 0 && Chance >= 40) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y - 1;
							data3.punchX = x;
							data3.punchY = y - 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5154;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5154, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width - worlds.at(iss)->width)).foreground = 5154;
						}
						else if (y < 59 && worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground == 0 && Chance >= 25) {
							PlayerMoving data3{};
							data3.packetType = 0x3;
							data3.characterState = 0x0;
							data3.x = x;
							data3.y = y + 1;
							data3.punchX = x;
							data3.punchY = y + 1;
							data3.XSpeed = 0;
							data3.YSpeed = 0;
							data3.netID = -1;
							data3.plantingTree = 5154;
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, worlds.at(iss)->name)) {
									SendParticleEffect(currentPeer6, x * 32, y * 32 + 16, 5154, 97, 0);
									auto raw = packPlayerMoving(&data3);
									raw[2] = 0;
									raw[3] = 0;
									SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
								}
							}
							worlds.at(iss)->items.at(x + (y * worlds.at(iss)->width + worlds.at(iss)->width)).foreground = 5154;
						}
						jas += 1;
					}
				}
			}
		}
		for (auto i = 0; i < worlds.size(); i++) {
			if (loop_freeze) break;
			//if (find(offloaded_worlds.begin(), offloaded_worlds.end(), worlds.at(i)->name) != offloaded_worlds.end()) break;
			if (i >= worlds.size()) {
				SendConsole("worlds.size() was larger than loop size (c797)", "ERROR");
				continue;
			}
			if (worlds.at(i)->name == "error" || worlds.at(i)->name == "EXIT") continue;		
			if (getPlayersCountInWorldEvent(worlds.at(i), worlds.at(i)->name) <= 0) continue; /*to avoid events in empty worlds*/
			if (getPlayersCountInWorldEvent(worlds.at(i), worlds.at(i)->name) >= 5) {
				bool NoEvent = false;
				for (auto jsss = 0; jsss < worlds.at(i)->width * worlds.at(i)->height; jsss++) {
					if (worlds.at(i)->items[jsss].foreground == 10422 && worlds.at(i)->items[jsss].activated) {
						NoEvent = true;
						break;
					}
				} if (NoEvent) {
					continue;
				}
				/*Choose event type*/
				string Notification = "";
				auto ItemID = 0;
				auto Count = 0;
				const auto Time = 0; /*Not used for now*/
				auto Multiple = 0; /*How much times the drop will be repeated*/
				int values;
				srand(time(nullptr));
				if (HarvestEvent) values = rand() % 15 + 1;
				else if (ValentineEvent) values = rand() % 30 + 1;
				else values = rand() % 13 + 1;
				switch (values)
				{
				case 1: /*World Lock Event*/
				{
					Notification = "`2World Locks Rain! `oYou have `wUnlimited `oseconds to find the `w3 `#World Locks `oHidden in this world!";
					ItemID = 242;
					Count = 1;
					Multiple = 3;
					break;
				}
				case 2: /*Surgery Split Event*/
				{
					Notification = "`2Surgical Split! `oYou have `wUnlimited `oseconds to find the `#Surgery Tool Pack `oHidden in this world!";
					ItemID = 9644;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 3: /*The Fireworks Event*/
				{
					Notification = "`2Bright Lights! `oYou have `wUnlimited `oseconds to find the `w5 `#Fireworks `oHidden in this world!";
					ItemID = 834;
					Count = 1;
					Multiple = 5;
					break;
				}
				case 4: /*Beat The Heat!*/
				{
					Notification = "`2Beat The Heat! `oYou have `wUnlimited `oseconds to find the `w5 `#Water Buckets `oHidden in this world!";
					ItemID = 822;
					Count = 1;
					Multiple = 5;
					break;
				}
				case 5: /*Beat The Heat!*/
				{
					Notification = "`2Beat The Heat! `oYou have `wUnlimited `oseconds to find the `w5 `#Water Buckets `oHidden in this world!";
					ItemID = 822;
					Count = 1;
					Multiple = 5;
					break;
				}
				case 6: /*Luck of the Growish Event*/
				{
					Notification = "`2Luck of the Growish! `oYou have `wUnlimited `oseconds to find the `#Lucky Clover `oHidden in this world!";
					ItemID = 528;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 7: /*Beautiful Crystal Event*/
				{
					Notification = "`2Beautiful Crystal! `oYou have `wUnlimited `oseconds to find the `#Crystal Block Seed `oHidden in this world!";
					ItemID = 263;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 8: /*Magical Seeds Event*/
				{
					Notification = "`2Beautiful Crystal! `oYou have `wUnlimited `oseconds to find the `#Crystal Block Seed `oHidden in this world!";
					ItemID = 263;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 9: /*Luck of the Growish Event*/
				{
					Notification = "`2Luck of the Growish! `oYou have `wUnlimited `oseconds to find the `#Lucky Clover `oHidden in this world!";
					ItemID = 528;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 10: /*Shinny Potion*/
				{
					Notification = "`2Shinny Potion! `oYou have `wUnlimited `oseconds to find the `#Punch Potion `oHidden in this world!";
					ItemID = 6918;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 11: /*What's that?*/
				{
					Notification = "`2What's that?! `oYou have `wUnlimited `oseconds to find the `#Doohickey `oHidden in this world!";
					ItemID = 1396;
					Count = 1;
					Multiple = 1;
					break;
				}
				case 12: /*Egg Hunt*/
				{
					Notification = "`2Egg Hunt! `oGo find them eggs! `w20 `#Magic Eggs `owill spawn in the world!";
					ItemID = 611;
					Count = 1;
					Multiple = 20;
					break;
				}
				case 13: /*Jackpot*/
				{
					Notification = "`2Jackpot! `oGems are spawned throughout the world!";
					ItemID = 112;
					Count = 1;
					Multiple = 100;
					break;
				}
				case 14: /*Songpyeon Hunt (harvest fest only)*/
				{
					Notification = "`2Songpyeon Hunt! `oYou have `wUnlimited `oseconds to find the `w3 `#Songpyeons `oHidden in this world!";
					ItemID = 1056;
					Count = 1;
					Multiple = 3;
					break;
				}
				case 15: /*Mooncake Rain (harvest fest only)*/
				{
					vector<int> CakeBase{ 1058, 1094, 1096, 1098, 1828, 3870, 7058 };
					const int Index = rand() % CakeBase.size();
					const auto IID = CakeBase[Index];
					Notification = "`2Mooncake Rain! `oYou have `wUnlimited `oseconds to find the `#" + getItemDef(IID).name + " `oHidden in this world!";
					ItemID = IID;
					Count = 1;
					Multiple = 5;
					break;
				}
				case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: /*Share The Love (valentine event only)*/
				{
					Notification = "`2Share The Love!: `oCelebrite Valentine's with free treasure!";
					bool success = true;
					for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
					{
						if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
						if (EventWorld(currentPeer, worlds.at(i)->name)) /*Display message for all players in world*/
						{
							SaveItemMoreTimes(3402, 1, currentPeer, success, static_cast<PlayerInfo*>(currentPeer->data)->rawName + " from special valentine event");
							SendTradeEffect(currentPeer, 3402, static_cast<PlayerInfo*>(currentPeer->data)->netID, static_cast<PlayerInfo*>(currentPeer->data)->netID, 150);
						}
					}
					break;
				}
				default: break;
				}
				for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer)
				{
					if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
					if (EventWorld(currentPeer, worlds.at(i)->name)) /*Display message for all players in world*/
					{
						Player::OnAddNotification(currentPeer, Notification, "audio/cumbia_horns.wav", "interface/large/special_event.rttex");
					}
				}
				if (ItemID != 0) SpawnEventItem(worlds.at(i)->name, ItemID, Count, Time, worlds.at(i)->width, worlds.at(i)->height, Multiple); /*Spawn the Item*/
				break;
			}
		}
	} catch(const std::out_of_range& e) {
		std::cout << e.what() << std::endl;
	} 
}

inline void WorldPhysics() {
	while (true) {
		Sleep(1000);
		try {
			vector<string> visi_galimi;
			for (int i = 0; i < premium_worlds.size(); i++) {
				visi_galimi.push_back(premium_worlds.at(i));
			} for (int i = 0; i < diamond_worlds.size(); i++) {
				visi_galimi.push_back(diamond_worlds.at(i));
			} for (int i = 0; i < platinum_worlds.size(); i++) {
				visi_galimi.push_back(platinum_worlds.at(i));
			} for (int iss = 0; iss < visi_galimi.size(); iss++) {									
				WorldInfo* world;
				string name = visi_galimi.at(iss);
				const auto p = std::find_if(worlds.begin(), worlds.end(), [name](const WorldInfo& a) { return a.name == name; });									
				if (p != worlds.end()) {
					world = &worlds.at(p - worlds.begin());
				} else if (std::find(diamond_worlds.begin(), diamond_worlds.end(), name) != diamond_worlds.end()) {
					world = worldDB.get_pointer(name);
				} else {
					continue;
				}
				if (getPlayersCountInWorldEvent(world, world->name) <= 0 && std::find(diamond_worlds.begin(), diamond_worlds.end(), name) == diamond_worlds.end()) continue;
				if (world->width > 150 || world->height > 100) continue;
				for (auto jas = 0; jas < world->width * world->height; jas++) {
					auto x = (jas % world->width);
					auto y = (jas / world->width);
					if (x < 0 || y < 0 || x > world->width - 1 || y > world->height - 1) continue;			
					if (x / 32 >= 0 && y / 32 >= 0 && x / 32 <= world->width && y / 32 <= world->height) {
						/*if (world->items.at(x + (y * world->width)).foreground == 3072) {
							for (auto magics = 0; magics < world->width * world->height; magics++) {
								int whyimgayx = (magics % world->width);
								int whyimgayy = (magics / world->width);
								if (whyimgayx < 0 || whyimgayy < 0 || whyimgayx > world->width - 1 || whyimgayy > world->height - 1) continue;
								if (whyimgayx / 32 >= 0 && whyimgayy / 32 >= 0 && whyimgayx / 32 <= world->width && whyimgayy / 32 <= world->height) {
									if (whyimgayy == 59 || whyimgayy == 99 || world->width == 60 && world->height == 30 && whyimgayy == 29) continue; 
									if (world->items.at(whyimgayx + (whyimgayy * world->width + world->width)).fire) {
										world->items.at(whyimgayx + (whyimgayy * world->width + world->width)).fire = false;
										for (ENetPeer* net_peersss = server->peers; net_peersss < &server->peers[server->peerCount]; ++net_peersss) {
											if (net_peersss->state != ENET_PEER_STATE_CONNECTED) continue;
											if (EventWorld(net_peersss, world->name)) {
												UpdateBlockState(net_peersss, whyimgayx, whyimgayy + 1, false, world);
												break;
											}
										}
										for (ENetPeer* net_peers = server->peers; net_peers < &server->peers[server->peerCount]; ++net_peers) {
											if (net_peers->state != ENET_PEER_STATE_CONNECTED) continue;
											if (EventWorld(net_peers, world->name)) {
												Player::OnParticleEffect(net_peers, 149, whyimgayx * 32, whyimgayy * 32 + 14, 0);
											}
										}
										break;
									}
								}
							}
						}*/
						if (world->items.at(x + (y * world->width)).foreground == 6954) {
							int PlaceID = 0;
							int IdentificatorID = world->items.at(x + (y * world->width)).vid;
							if (world->items.at(x + (y * world->width)).mid != 0 && world->items.at(x + (y * world->width)).mc > 0) {
								bool Imanoma = false;
								PlaceID = world->items.at(x + (y * world->width)).mid;
								for (auto magics = 0; magics < world->width * world->height; magics++) {
									int whyimgayx = (magics % world->width);
									int whyimgayy = (magics / world->width);
									if (whyimgayx < 0 || whyimgayy < 0 || whyimgayx > world->width - 1 || whyimgayy > world->height - 1) continue;
									if (whyimgayx / 32 >= 0 && whyimgayy / 32 >= 0 && whyimgayx / 32 <= world->width && whyimgayy / 32 <= world->height) {
										if (whyimgayy == 59 || whyimgayy == 99 || world->width == 60 && world->height == 30 && whyimgayy == 29) continue; /*nezinia*/
										if (world->items.at(whyimgayx + (whyimgayy * world->width + world->width)).foreground == IdentificatorID && world->items.at(whyimgayx + (whyimgayy * world->width)).foreground == 0) {
											Imanoma = true;
											break;
										} 
									}
								}
								if (!Imanoma) continue;						
								world->items.at(x + (y * world->width)).mc -= 1;
								if (world->items.at(x + (y * world->width)).mc <= 0) {
									thread_safe_magupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(6954) + "|" + to_string(world->items.at(x + (y * world->width)).mid) + "|" + to_string(-1) + "|" + to_string(world->items.at(x + (y * world->width)).background));																		
								} else {
									thread_safe_magupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(6954) + "|" + to_string(world->items.at(x + (y * world->width)).mid) + "|" + to_string(1) + "|" + to_string(world->items.at(x + (y * world->width)).background));									
								}
							} else if (world->items.at(x + (y * world->width)).rm) {
								bool Imanoma = false;
								for (auto magics = 0; magics < world->width * world->height; magics++) {
									int whyimgayx = (magics % world->width);
									int whyimgayy = (magics / world->width);
									if (whyimgayx < 0 || whyimgayy < 0 || whyimgayx > world->width - 1 || whyimgayy > world->height - 1) continue;
									if (whyimgayx / 32 >= 0 && whyimgayy / 32 >= 0 && whyimgayx / 32 <= world->width && whyimgayy / 32 <= world->height) {
										if (whyimgayy == 59 || whyimgayy == 99 || world->width == 60 && world->height == 30 && whyimgayy == 29) continue; /*nezinia*/
										if (world->items.at(whyimgayx + (whyimgayy * world->width + world->width)).foreground == IdentificatorID && world->items.at(whyimgayx + (whyimgayy * world->width)).foreground == 0) {
											Imanoma = true;
											break;
										} 
									}
								}
								if (!Imanoma) continue;
								for (auto magics = 0; magics < world->width * world->height; magics++) {
									int whyimgayx = (magics % world->width);
									int whyimgayy = (magics / world->width);												
									if (whyimgayx < 0 || whyimgayy < 0 || whyimgayx > world->width - 1 || whyimgayy > world->height - 1) continue;
									if (whyimgayx / 32 >= 0 && whyimgayy / 32 >= 0 && whyimgayx / 32 <= world->width && whyimgayy / 32 <= world->height) {
										if (whyimgayy == 59 || whyimgayy == 99 || world->width == 60 && world->height == 30 && whyimgayy == 29) continue; /*nezinia*/
										if (world->items.at(whyimgayx + (whyimgayy * world->width)).foreground == 5638 && world->items.at(whyimgayx + (whyimgayy * world->width)).mid == world->items.at(x + (y * world->width)).mid && world->items.at(whyimgayx + (whyimgayy * world->width)).mc > 0) {
											PlaceID = world->items.at(whyimgayx + (whyimgayy * world->width)).mid;
											world->items.at(whyimgayx + (whyimgayy * world->width)).mc -= 1;
											if (world->items.at(whyimgayx + (whyimgayy * world->width)).mc <= 0) {
												thread_safe_magupdate.push_back(world->name + "|" + to_string(whyimgayx) + "|" + to_string(whyimgayy) + "|" + to_string(5638) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).mid) + "|" + to_string(0) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).background));									
											} else if (world->items.at(whyimgayx + (whyimgayy * world->width)).mc >= 5000) {
												thread_safe_magupdate.push_back(world->name + "|" + to_string(whyimgayx) + "|" + to_string(whyimgayy) + "|" + to_string(5638) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).mid) + "|" + to_string(-1) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).background));									
											} else {
												thread_safe_magupdate.push_back(world->name + "|" + to_string(whyimgayx) + "|" + to_string(whyimgayy) + "|" + to_string(5638) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).mid) + "|" + to_string(1) + "|" + to_string(world->items.at(whyimgayx + (whyimgayy * world->width)).background));									
											}
											break; /*and move on*/
										}
									}
								}
							}
							if (PlaceID != 0) {
								for (auto magic = 0; magic < world->width * world->height; magic++) {																		
									x = (magic % world->width);
									y = (magic / world->width);
									if (x < 0 || y < 0 || x > world->width - 1 || y > world->height - 1) continue;
									if (x / 32 >= 0 && y / 32 >= 0 && x / 32 <= world->width && y / 32 <= world->height) {
										if (world->items.at(x + (y * world->width + world->width)).foreground == IdentificatorID && world->items.at(x + (y * world->width)).foreground == 0) {
											thread_safe_effects.push_back(world->name + "|229|" + to_string(x) + "|" + to_string(y));
											world->items.at(x + (y * world->width)).foreground = PlaceID;
											bool UpdateTreeVisuals3 = false;
											if (isSeed(PlaceID)) {
												UpdateTreeVisuals3 = true;
												int growTimeSeed = getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
												growTimeSeed += 30 * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
												world->items.at(x + (y * world->width)).growtime = (GetCurrentTimeInternalSeconds() + growTimeSeed);
												if (getItemDef(world->items.at(x + (y * world->width)).foreground).rarity == 999) {
													world->items.at(x + (y * world->width)).fruitcount = (rand() % 1) + 1;
												} else {
													world->items.at(x + (y * world->width)).fruitcount = (rand() % 5) + 1;									
												}
												if (getItemDef(world->items.at(x + (y * world->width)).foreground - 1).blockType == BlockTypes::CLOTHING) world->items.at(x + (y * world->width)).fruitcount = (rand() % 2) + 1;
												spray_tree_wopeer(world, x, y, 18, true);
											}
											int InternalGrowTime = 0;
											int growTimeSeed = getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
											growTimeSeed += 30 * getItemDef(world->items.at(x + (y * world->width)).foreground - 1).rarity;
											InternalGrowTime = growTimeSeed - calcBanDuration(world->items.at(x + (y * world->width)).growtime);
											thread_safe_tileupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(PlaceID));									
											if (UpdateTreeVisuals3) {
												thread_safe_seedupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(world->items.at(x + (y * world->width)).foreground) + "|" + to_string(world->items.at(x + (y * world->width)).background) + "|" + to_string(world->items.at(x + (y * world->width)).fruitcount) + "|" + to_string(InternalGrowTime));
											}
											
											break;
										}
									}
								}
							}
						}
						if (world->items.at(x + (y * world->width)).foreground == 6952 && world->items.at(x + (y * world->width)).mid != 0) {
							int destroyid = world->items.at(x + (y * world->width)).mid;				
							for (auto magic = 0; magic < world->width * world->height; magic++) {
								x = (magic % world->width);
								y = (magic / world->width);
								if (x < 0 || y < 0 || x > world->width - 1 || y > world->height - 1) continue;
								if (x / 32 >= 0 && y / 32 >= 0 && x / 32 <= world->width && y / 32 <= world->height) {
									if (!world->items.at(x + (y * world->width)).destroy && world->items.at(x + (y * world->width)).foreground == destroyid || world->items.at(x + (y * world->width)).background == destroyid) {
										if (isSeed(world->items.at(x + (y * world->width)).foreground) && calcBanDuration(world->items.at(x + (y * world->width)).growtime) != 0) continue;
										thread_safe_effects.push_back(world->name + "|229|" + to_string(x) + "|" + to_string(y));
										auto count = 0;
										for (auto currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
											if (currentPeer->state != ENET_PEER_STATE_CONNECTED || currentPeer->data == NULL) continue;
											if (static_cast<PlayerInfo*>(currentPeer->data)->currentWorld == world->name) {
												count++;
											}
										}
										if (count <= 0) {
											if (!world->items.at(x + (y * world->width)).destroy && isSeed(world->items.at(x + (y * world->width)).foreground) || !world->items.at(x + (y * world->width)).destroy && world->items.at(x + (y * world->width)).breakbotstate >= getItemDef(world->items.at(x + (y * world->width)).foreground).breakHits) {
												world->items.at(x + (y * world->width)).breakbotstate = 0;
												world->items.at(x + (y * world->width)).destroy = true;
											} else {
												world->items.at(x + (y * world->width)).breakbotstate++;
											}											
										} else {			
											thread_safe_punchupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y));   
										}
										break;
									}
								}
							}
						}
						/*if (isSeed(world->items.at(x + (y * world->width)).foreground) && calcBanDuration(world->items.at(x + (y * world->width)).growtime) == 0) {
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, world->name)) {
									UpdateTreeVisuals(currentPeer6, world->items.at(x + (y * world->width)).foreground, x, y, world->items.at(x + (y * world->width)).background, world->items.at(x + (y * world->width)).fruitcount, 999999999, false, 0);
								}
							}
						}*/
						/*if (world->items.at(x + (y * world->width)).activated) {
							for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
								if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
								if (EventWorld(currentPeer6, world->name)) {
									UpdateBlockState(currentPeer6, x, y, true, world);
								}
							}
						}*/
						/*if (world->items.at(x + (y * world->width)).foreground == 9148 && world->items.at(x - 1 + (y * world->width)).foreground == 0 && !world->items.at(x + (y * world->width)).atnaujino) {
							if (world->items.at(x - 1 + (y * world->width)).foreground == 0) {
								world->items.at(x + (y * world->width)).atnaujino = false;
								world->items.at(x - 1 + (y * world->width)).atnaujino = true;
								world->items.at(x - 1 + (y * world->width)).foreground = 9148;
								PlayerMoving data3{};
								data3.packetType = 0x3;
								data3.characterState = 0x0;
								data3.x = x - 1;
								data3.y = y;
								data3.punchX = x - 1;
								data3.punchY = y;
								data3.XSpeed = 0;
								data3.YSpeed = 0;
								data3.netID = -1;
								data3.plantingTree = 9148;
								for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
									if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
									if (EventWorld(currentPeer6, world->name)) {
										auto raw = packPlayerMoving(&data3);
										raw[2] = 0;
										raw[3] = 0;
										SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
									}
								}
							}
						}
						if (world->items.at(x + (y * world->width)).foreground == 9148 && world->items.at(x + 1 + (y * world->width)).foreground == 0 && !world->items.at(x + (y * world->width)).atnaujino) {
							if (world->items.at(x + 1 + (y * world->width)).foreground == 0) {
								world->items.at(x + (y * world->width)).atnaujino = false;
								world->items.at(x + 1 + (y * world->width)).atnaujino = true;
								world->items.at(x + 1 + (y * world->width)).foreground = 9148;
								PlayerMoving data3{};
								data3.packetType = 0x3;
								data3.characterState = 0x0;
								data3.x = x + 1;
								data3.y = y;
								data3.punchX = x + 1;
								data3.punchY = y;
								data3.XSpeed = 0;
								data3.YSpeed = 0;
								data3.netID = -1;
								data3.plantingTree = 9148;
								for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
									if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
									if (EventWorld(currentPeer6, world->name)) {
										auto raw = packPlayerMoving(&data3);
										raw[2] = 0;
										raw[3] = 0;
										SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
									}
								}
							}
						}
						if (world->items.at(x + (y * world->width)).foreground == 9148 && world->items.at(x + (y * world->width + world->width)).foreground == 0 && !world->items.at(x + (y * world->width)).atnaujino) {
							if (world->items.at(x + (y * world->width + world->width)).foreground == 0) {
								world->items.at(x + (y * world->width)).atnaujino = false;
								world->items.at(x + (y * world->width + world->width)).atnaujino = true;
								world->items.at(x + (y * world->width + world->width)).foreground = 9148;
								PlayerMoving data3{};
								data3.packetType = 0x3;
								data3.characterState = 0x0;
								data3.x = x;
								data3.y = y + 1;
								data3.punchX = x;
								data3.punchY = y + 1;
								data3.XSpeed = 0;
								data3.YSpeed = 0;
								data3.netID = -1;
								data3.plantingTree = 9148;
								for (ENetPeer* currentPeer6 = server->peers; currentPeer6 < &server->peers[server->peerCount]; ++currentPeer6) {
									if (currentPeer6->state != ENET_PEER_STATE_CONNECTED) continue;
									if (EventWorld(currentPeer6, world->name)) {
										auto raw = packPlayerMoving(&data3);
										raw[2] = 0;
										raw[3] = 0;
										SendPacketRaw(4, raw, 56, nullptr, currentPeer6, ENET_PACKET_FLAG_RELIABLE);
									}
								}
							}
						}*/
						if (world->items.at(x + (y * world->width)).foreground == 442 && world->items.at(x + (y * world->width + world->width)).foreground == 0 && !world->items.at(x + (y * world->width)).atnaujino) {
							if (world->items.at(x + (y * world->width + world->width)).foreground == 0) {
								world->items.at(x + (y * world->width)).atnaujino = false;
								world->items.at(x + (y * world->width + world->width)).atnaujino = true;
								int beforeid = world->items.at(x + (y * world->width)).foreground;
								world->items.at(x + (y * world->width)).foreground = 0;
								thread_safe_tileupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y) + "|" + to_string(0));									
								world->items.at(x + (y * world->width + world->width)).foreground = beforeid;
								thread_safe_tileupdate.push_back(world->name + "|" + to_string(x) + "|" + to_string(y + 1) + "|" + to_string(beforeid));									
							}
						}
					}
					if (jas == (world->width * world->height) - 1) {
						for (auto magic = 0; magic < world->width * world->height; magic++) {
							x = (magic % world->width);
							y = (magic / world->width);
							if (x / 32 >= 0 && y / 32 >= 0 && x / 32 <= world->width && y / 32 <= world->height) {
								world->items.at(x + (y * world->width)).atnaujino = false;
							}
						}
						break;
					}
				}
			}
		} catch(const std::out_of_range& e) {
			std::cout << e.what() << std::endl;
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		} catch(...) {
			std::cout << "error" << std::endl;
		} 
	}
}
