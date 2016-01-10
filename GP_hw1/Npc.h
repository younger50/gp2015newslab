#ifndef _NPC_H_
#define _NPC_H_

#include "FlyWin32.h"
#include <vector>
#include <list>
#include <string>
using namespace std;

#define OPEN 0
#define CLOSED 1

class Npc{
public:
	Npc(){};
	~Npc(){};

	// npc id
	CHARACTERid ID;

	//npc name
	string name;

	// npc action
	ACTIONid IdleID, RunID, CurPoseID;
	ACTIONid NormalAttack1ID, NormalAttack2ID, NormalAttack3ID;
	ACTIONid HeavyAttack1ID;
	ACTIONid GuardID;
	ACTIONid Damage1ID, Damage2ID, DieID;

	// npc hp
	int HealthPoints = 100;
	OBJECTid hpid;
	GEOMETRYid hpboardid;
	
	// npc movement
	list<float*> pre_pos;
	static const int movespeed = 7;
	static const int box_size = 30;

	// npc attack
	float attackrate = 0.3f;
	int attack_counter = 0;
	bool AlreadyBeenHit = false;
	bool running = false, canAttack = false;
};

#endif