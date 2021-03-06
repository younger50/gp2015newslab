/*==============================================================
character movement testing using Fly2

- Load a scene
- Generate a terrain object
- Load a character
- Control a character to move
- Change poses

(C)2012-2015 Chuan-Chang Wang, All Rights Reserved
Created : 0802, 2012

Last Updated : 1004, 2015, Kevin C. Wang
===============================================================*/
#include "FlyWin32.h"
#include "RayTracer.h"
#include "Npc.h"
#include <string>
#include "Projectile.h"
using namespace std;

char debugbuf[256]; // debug UI message buffer
float pi = 3.14;

VIEWPORTid vID;                 // the major viewport
SCENEid sID;                    // the 3D scene
OBJECTid cID, tID;              // the main camera and the terrain for terrain following


CHARACTERid actorID;            // the major character
int actorAttacking = 0, actorAttackFrame = 0; // actor global
int stack = 0; // keep track of multi key press
int pause = 0;					// game pause status

FnObject Kagemusha;

//sound
AUDIOid background_sound_id;
FnAudio audobj;


//hp
OBJECTid actor_hpid;
GEOMETRYid actor_hpboardid;

// focus
OBJECTid fcid;
GEOMETRYid fcbid;
// fx
GAMEFX_SYSTEMid lyfxID;
OBJECTid lyfxdumID;

// actor = lyubu
ACTIONid IdleID, RunID, WalkID, CurPoseID;
ACTIONid NormalAttack1ID, NormalAttack2ID, NormalAttack3ID, NormalAttack4ID; 
ACTIONid HeavyAttack1ID, HeavyAttack2ID, HeavyAttack3ID;
ACTIONid UltimateAttackID;
ACTIONid GuardID;
ACTIONid HeavyDamageID, RightDamageID, LeftDamageID, DieID;
int actor_HealthPoints = 100;
bool actor_AlreadyHit = false;

unsigned int AttackRange = 100;
float AttackRadius = 1.0f;
float skill_camera_timer = 0;
bool isCombo = false;
ACTIONid NextAttackID;

void PlayActorAction(int skip); // play actor action frame by frame

void isNPCHit();
void isNPCHitUltimate();

void playmusic(FnAudio, char *, float);
void NPCattackActor(int number);
void NPCAttackActorRange(int number);
int NPCcollideToOther(int number, float* pos);


// npc Object array
#define NPC_NUMBER 6
Npc npc[NPC_NUMBER+1];	//start from 1 (0 is not use)

//vector storing Projectiles
vector<Projectile *> projectiles;

void projectileUpdate(int skip);


ROOMid terrainRoomID = FAILED_ID;
TEXTid textID = FAILED_ID;

// some globals
int frame = 0;
int oldX, oldY, oldXM, oldYM, oldXMM, oldYMM;

// hotkey callbacks
void QuitGame(BYTE, BOOL4);
void Movement(BYTE, BOOL4);
void ActorAttack(BYTE, BOOL4);
void ActorDefence(BYTE, BOOL4);

// menu 
void StartMenu(int);
void EndMenu(int);
void EndKey(BYTE, BOOL4);
int not_end = 1;

// npc movement
#define OPEN 0
#define CLOSED 1
struct Node{
	struct Node *parent;
	float pos[3];
	float gn;	// distance between Start and this node(step distance)
	float hn;	// distance between Start and Goal
	float fn;	// gn + hn
	bool status;
};
Node* NPCmovement(int number);
vector<Node*> open, close, nodelist;

// timer callbacks
void GameAI(int);
void RenderIt(int);

// camera view 
void Camera3PersonView(float);

// mouse callbacks
void InitPivot(int, int);
void PivotCam(int, int);
void InitMove(int, int);
void MoveCam(int, int);
void InitZoom(int, int);
void ZoomCam(int, int);

// collision cam mod
void CameraCollision();

// rayTracer
RayTracer rayTracer;

// 3D vector cross
void cross3(float *answer, float *a, float *b){
	// catious!! no boundary check
	answer[0] = a[1] * b[2] - a[2] * b[1];
	answer[1] = -a[0] * b[2] + a[2] * b[0];
	answer[2] = a[0] * b[1] - a[1] * b[0];
};

// 3D point distance
float dist3(float *a, float *b){
	return (a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]) + (a[2] - b[2])*(a[2] - b[2]);
}

float dist2(float *a, float *b){
	return sqrt((a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]));
}

/*------------------
the main program
C.Wang 1010, 2014
-------------------*/
void FyMain(int argc, char **argv)
{
	// create a new world
	BOOL4 beOK = FyStartFlyWin32("/NTU Game Programming 2015 Final Project - Use Fly2", 0, 0, 1024, 768, FALSE);

	// setup the data searching paths
	FySetShaderPath("Data\\NTU6\\Shaders");
	FySetModelPath("Data\\NTU6\\Scenes");
	FySetTexturePath("Data\\NTU6\\Scenes\\Textures");
	FySetScenePath("Data\\NTU6\\Scenes");
	FySetGameFXPath("Data\\NTU6\\FX");

	// create a viewport
	vID = FyCreateViewport(0, 0, 1024, 768);
	FnViewport vp;
	vp.ID(vID);

	// create a 3D scene
	sID = FyCreateScene(10);
	FnScene scene;
	scene.ID(sID);

	// load the scene
	scene.Load("gameScene02");
	scene.SetAmbientLights(1.0f, 1.0f, 1.0f, 0.6f, 0.6f, 0.6f);

	// load the terrain
	tID = scene.CreateObject(OBJECT);
	FnObject terrain;
	terrain.ID(tID);
	BOOL beOK1 = terrain.Load("terrain");
	terrain.Show(FALSE);

	//lyubu's hp
	float actor_hpsize[2] = { 50, 5 };
	FnObject actor_hpobj;
	actor_hpid = scene.CreateObject(OBJECT);
	actor_hpobj.ID(actor_hpid);
	actor_hpobj.Show(TRUE);
	actor_hpboardid = actor_hpobj.Billboard(NULL, actor_hpsize, "Data\\NTU6\\NPCs\\actor_hp", 0);

	//npc's hp
	for (int i = 1; i <= NPC_NUMBER; i++){
		float npc_hpsize[2] = { 50, 5 };
		FnObject npchpobj;
		npc[i].hpid = scene.CreateObject(OBJECT);
		npchpobj.ID(npc[i].hpid);
		npchpobj.Show(TRUE);
		npc[i].hpboardid = npchpobj.Billboard(NULL, npc_hpsize, "Data\\NTU6\\NPCs\\hp", 0);
	}

	// focus
	float fcsize[2] = { 200, 150 };
	FnObject fcobj;
	fcid = scene.CreateObject(OBJECT);
	fcobj.ID(fcid);
	fcobj.Show(FALSE);
	//fcobj.SetOpacity(0.5);
	fcobj.SetAlphaFlag(TRUE);
	fcbid = fcobj.Billboard(NULL, fcsize, "Data\\NTU6\\FXhw\\f1", 0);

	// create a new game FX system
	lyfxID = scene.CreateGameFXSystem();
	lyfxdumID = scene.CreateObject();

	// creat menu

	// set terrain environment
	terrainRoomID = scene.CreateRoom(SIMPLE_ROOM, 10);
	FnRoom room;
	room.ID(terrainRoomID);
	room.AddObject(tID);

	


	// load the character
	FySetModelPath("Data\\NTU6\\Characters");
	FySetTexturePath("Data\\NTU6\\Characters");
	FySetCharacterPath("Data\\NTU6\\Characters");
	actorID = scene.LoadCharacter("Lyubu2");
	npc[1].ID = scene.LoadCharacter("Donzo2");
	npc[1].name = "Donzo2";
	npc[2].ID = scene.LoadCharacter("Robber02");

	npc[2].name = "Robber02";
	npc[3].ID = scene.LoadCharacter("Robber02");
	npc[3].name = "Robber02";
	

	FySetModelPath("Data\\NTU6\\NPCs");
	FySetTexturePath("Data\\NTU6\\NPCs");
	FySetCharacterPath("Data\\NTU6\\NPCs");
	npc[4].ID = scene.LoadCharacter("CA002");
	npc[4].name = "CA002";
	npc[5].ID = scene.LoadCharacter("CA003");
	npc[5].name = "CA003";
	npc[6].ID = scene.LoadCharacter("CA004");
	npc[6].name = "CA004";

	FySetScenePath("Data\\NTU6\\Scenes");
	FySetAudioPath("Data\\NTU6\\Media");	
	
	FnAudio background_sound;

	background_sound.ID(FyCreateAudio());
	background_sound.Load("background");

	background_sound.SetVolume(0.1);
	background_sound.Play(LOOP);
	
	
	
	// put the character on terrain
	float pos[3], fDir[3], uDir[3];
	FnCharacter actor;
	actor.ID(actorID);
	pos[0] = 3598.0f; pos[1] = -3907.0f; pos[2] = 1000.0f;
	fDir[0] = -0.284f; fDir[1] = -0.954f; fDir[2] = -0.1f;
	uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
	actor.SetDirection(fDir, uDir);
	actor.SetTerrainRoom(terrainRoomID, 10.0f);
	beOK = actor.PutOnTerrain(pos);

	float dir[3], npos[3];
	npos[0] = pos[0] + 50;
	npos[1] = pos[1] + 50;
	npos[2] = pos[2];

	dir[0] = pos[0] - npos[0];
	dir[1] = pos[1] - npos[1];
	dir[2] = 20;

	Projectile* p = new Projectile(dir, 100, 20, npos, sID, "Data\\NTU6\\FX\\UltSwin");
	projectiles.push_back(p);
	
	for (int i = 1; i <= NPC_NUMBER; i++){
		float pos[3], fDir[3], uDir[3];
		if (i == 1){
			//donzo
			pos[0] = 2769.0f; pos[1] = -2708.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}
		else if (i == 2){
			//robber
			pos[0] = 3569.0f; pos[1] = -2908.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}
		else if (i == 3){
			//robber
			pos[0] = 3000.0f; pos[1] = -2708.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}
		else if (i == 4){
			//CA002
			pos[0] = 3269.0f; pos[1] = -2608.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}
		else if (i == 5){
			//CA003
			pos[0] = 3600.0f; pos[1] = -2708.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}
		else if (i == 6){
			//CA004
			pos[0] = 3500.0f; pos[1] = -2908.0f; pos[2] = 1000.0f;
			fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		}

		FnCharacter npcChar;
		npcChar.ID(npc[i].ID);
		npcChar.SetDirection(fDir, uDir);
		npcChar.SetTerrainRoom(terrainRoomID, 10.0f);
		beOK = npcChar.PutOnTerrain(pos);
	}

	// put Kagemusha on terrian
	Kagemusha.ID(scene.CreateObject(OBJECT));
	Kagemusha.SetDirection(fDir, uDir);
	Kagemusha.SetTerrainRoom(terrainRoomID, 10.0f);
	beOK = Kagemusha.PutOnTerrain(pos);

	// Get character actions pre-defined 
	IdleID = actor.GetBodyAction(NULL, "Idle");
	RunID = actor.GetBodyAction(NULL, "Run");
	WalkID = actor.GetBodyAction(NULL, "Walk");
	NormalAttack1ID = actor.GetBodyAction(NULL, "NormalAttack1");
	NormalAttack2ID = actor.GetBodyAction(NULL, "NormalAttack2");
	NormalAttack3ID = actor.GetBodyAction(NULL, "NormalAttack3");
	NormalAttack4ID = actor.GetBodyAction(NULL, "NormalAttack4");
	HeavyAttack1ID = actor.GetBodyAction(NULL, "HeavyAttack1");
	HeavyAttack2ID = actor.GetBodyAction(NULL, "HeavyAttack2");
	HeavyAttack3ID = actor.GetBodyAction(NULL, "HeavyAttack3");
	UltimateAttackID = actor.GetBodyAction(NULL, "UltimateAttack");
	GuardID = actor.GetBodyAction(NULL, "guard");
	HeavyDamageID = actor.GetBodyAction(NULL, "HeavyDamaged");
	RightDamageID = actor.GetBodyAction(NULL, "RightDamaged");
	LeftDamageID = actor.GetBodyAction(NULL, "LeftDamaged");
	DieID = actor.GetBodyAction(NULL, "Die");

	for (int i = 1; i <= NPC_NUMBER; i++){
		FnCharacter n;
		n.ID(npc[i].ID);
		if (npc[i].name.compare("Donzo2") == 0){
			npc[i].IdleID = n.GetBodyAction(NULL, "Idle");
			npc[i].RunID = n.GetBodyAction(NULL, "Run");
			npc[i].NormalAttack1ID = n.GetBodyAction(NULL, "AttackL1");
			npc[i].NormalAttack2ID = n.GetBodyAction(NULL, "AttackL2");
			npc[i].NormalAttack3ID = n.GetBodyAction(NULL, "AttackH");
			npc[i].HeavyAttack1ID = n.GetBodyAction(NULL, "HeavyAttack");
			npc[i].GuardID = n.GetBodyAction(NULL, "Defence");
			npc[i].Damage1ID = n.GetBodyAction(NULL, "DamageL");
			npc[i].Damage2ID = n.GetBodyAction(NULL, "DamageH");
			npc[i].DieID = n.GetBodyAction(NULL, "Die");
			npc[i].attack_range = 100;
			npc[i].attackType = "melee";
		}
		if (npc[i].name.compare("Robber02") == 0){
			npc[i].IdleID = n.GetBodyAction(NULL, "CombatIdle");
			npc[i].RunID = n.GetBodyAction(NULL, "Run");
			npc[i].NormalAttack1ID = n.GetBodyAction(NULL, "NormalAttack1");
			npc[i].NormalAttack2ID = n.GetBodyAction(NULL, "NormalAttack2");
			npc[i].HeavyAttack1ID = n.GetBodyAction(NULL, "HeavyAttack1");
			npc[i].Damage1ID = n.GetBodyAction(NULL, "Damage1");
			npc[i].Damage2ID = n.GetBodyAction(NULL, "Damage2");
			npc[i].DieID = n.GetBodyAction(NULL, "Dead");
			npc[i].attack_range = 100;
			npc[i].attackType = "melee";
		}
		if (npc[i].name.compare("CA002") == 0){
			//fuck
			npc[i].IdleID = n.GetBodyAction(NULL, "Idle");
			npc[i].RunID = n.GetBodyAction(NULL, "Run");
			npc[i].NormalAttack1ID = n.GetBodyAction(NULL, "Attack");
			npc[i].HeavyAttack1ID = n.GetBodyAction(NULL, "Skill");
			npc[i].Damage1ID = n.GetBodyAction(NULL, "Run");
			npc[i].DieID = n.GetBodyAction(NULL, "Die");
			npc[i].attack_range = 1000;
			npc[i].attackType = "range";
			strcpy(npc[i].attack_special, "Data\\NTU6\\FX\\CA002_atk01");
		}
		if (npc[i].name.compare("CA003") == 0){
			//zonda
			npc[i].IdleID = n.GetBodyAction(NULL, "Idle");
			npc[i].RunID = n.GetBodyAction(NULL, "Run");
			npc[i].NormalAttack1ID = n.GetBodyAction(NULL, "Attack");
			npc[i].HeavyAttack1ID = n.GetBodyAction(NULL, "Skill");
			npc[i].Damage1ID = n.GetBodyAction(NULL, "Run");
			npc[i].DieID = n.GetBodyAction(NULL, "Die");
			npc[i].attack_range = 1000;
			npc[i].attackType = "range";
			strcpy(npc[i].attack_special, "Data\\NTU6\\FX\\Redcircle01");
		}
		if (npc[i].name.compare("CA004") == 0){
			//girl
			npc[i].IdleID = n.GetBodyAction(NULL, "Idle");
			npc[i].RunID = n.GetBodyAction(NULL, "Run");
			npc[i].NormalAttack1ID = n.GetBodyAction(NULL, "Attack");
			npc[i].HeavyAttack1ID = n.GetBodyAction(NULL, "Skill");
			npc[i].Damage1ID = n.GetBodyAction(NULL, "Run");
			npc[i].DieID = n.GetBodyAction(NULL, "Die");
			npc[i].attack_range = 1000;
			npc[i].attackType = "range";
			strcpy(npc[i].attack_special, "Data\\NTU6\\FX\\LyubuDamege");
		}
	}
	

	// set the character to idle action
	CurPoseID = IdleID;
	actor.SetCurrentAction(NULL, 0, CurPoseID);
	actor.Play(START, 0.0f, FALSE, TRUE);
	actor.TurnRight(90.0f);

	for (int i = 1; i <= NPC_NUMBER; i++){
		FnCharacter npcChar;
		npcChar.ID(npc[i].ID);
		npcChar.SetCurrentAction(NULL, 0, npc[i].IdleID);
		npcChar.Play(START, 0.0f, FALSE, TRUE);
		npcChar.TurnRight(90.0f);
	}	

	// translate the camera
	cID = scene.CreateObject(CAMERA);
	FnCamera camera;
	camera.ID(cID);
	camera.SetNearPlane(5.0f);
	camera.SetFarPlane(100000.0f);

	// set camera initial position and orientation
	pos[0] = 3298.0f; pos[1] = -3907.0f; pos[2] = 93.046f;
	//pos[0] = 4315.783f; pos[1] = -3199.686f; pos[2] = 93.046f;
	fDir[0] = -0.284f; fDir[1] = -0.954f; fDir[2] = -0.1f;
	uDir[0] = -0.116f; uDir[1] = -0.031f; uDir[2] = 0.993f;
	camera.SetPosition(pos);
	camera.SetDirection(fDir, uDir);
	Camera3PersonView(0); // default face back

	// light
	float mainLightPos[3] = { -4579.0, -714.0, 15530.0 };
	float mainLightFDir[3] = { 0.276, 0.0, -0.961 };
	float mainLightUDir[3] = { 0.961, 0.026, 0.276 };




	FnLight lgt;
	lgt.ID(scene.CreateObject(LIGHT));
	lgt.Translate(mainLightPos[0], mainLightPos[1], mainLightPos[2], REPLACE);
	lgt.SetDirection(mainLightFDir, mainLightUDir);
	lgt.SetLightType(PARALLEL_LIGHT);
	lgt.SetColor(1.0f, 1.0f, 1.0f);
	lgt.SetName("MainLight");
	lgt.SetIntensity(0.4f);

	// create a text object for displaying messages on screen
	textID = FyCreateText("Trebuchet MS", 18, FALSE, FALSE);

	// set Hotkeys
	FyDefineHotKey(FY_ESCAPE, QuitGame, FALSE);  // escape for quiting the game
	FyDefineHotKey(FY_UP, Movement, FALSE);      // Up for moving forward
	FyDefineHotKey(FY_RIGHT, Movement, FALSE);   // Right for moving right
	FyDefineHotKey(FY_LEFT, Movement, FALSE);    // Left for moving left
	FyDefineHotKey(FY_DOWN, Movement, FALSE);	 // Down for moving back
	FyDefineHotKey(FY_Z, ActorAttack, FALSE);	 // Normal Attack
	FyDefineHotKey(FY_X, ActorAttack, FALSE);	 // Heavy Attack
	FyDefineHotKey(FY_C, ActorAttack, FALSE);	// Ultimate Attack
	FyDefineHotKey(FY_SPACE, ActorDefence, FALSE);    // Defand

	FyDefineHotKey(FY_P, EndKey, FALSE); // Pause main game 

	// define some mouse functions
	FyBindMouseFunction(LEFT_MOUSE, InitPivot, PivotCam, NULL, NULL);
	FyBindMouseFunction(MIDDLE_MOUSE, InitZoom, ZoomCam, NULL, NULL);
	FyBindMouseFunction(RIGHT_MOUSE, InitMove, MoveCam, NULL, NULL);

	// bind timers, frame rate = 30 fps
	//FyBindTimer(0, 30.0f, GameAI, TRUE);
	//FyBindTimer(1, 30.0f, RenderIt, TRUE);
	//FyBindTimer(2, 30.0f, projectileUpdate, TRUE);
	// start from menu
	FyBindTimer(5, 30.0f, StartMenu, TRUE);
	// FyBindTimer(2, 30.0f, Camera3PersonView, TRUE);
	// invoke the system
	FyInvokeFly(TRUE);
}

/*-------------------------------------------------------------
30fps timer callback in fixed frame rate for major game loop
--------------------------------------------------------------*/
void GameAI(int skip)
{
	FnCharacter actor;
	float speed = 10.0f;
	float rotate = 5.0f;

	// play character pose
	actor.ID(actorID);

	//==============================================
	//				character pose 
	//==============================================
	// play character pose
	CurPoseID = actor.GetCurrentAction(NULL);
	//sprintf(debugbuf, "skill op %d", CurPoseID);
	if (CurPoseID == IdleID || CurPoseID == RunID)
	{
		actor.Play(LOOP, (float)skip, FALSE, TRUE);
	}
	else if (CurPoseID == NormalAttack1ID ||
		CurPoseID == NormalAttack2ID ||
		CurPoseID == NormalAttack3ID ||
		CurPoseID == NormalAttack4ID ||
		CurPoseID == HeavyAttack1ID ||
		CurPoseID == HeavyAttack2ID ||
		CurPoseID == HeavyAttack3ID)
	{
		isNPCHit();
		//actor.Play(ONCE, (float)skip, TRUE);
		if (!actor.Play(ONCE, (float)skip, FALSE, TRUE, TRUE)){
			if (isCombo)
			{
				actor.SetCurrentAction(NULL, 0, NextAttackID);
				isCombo = false;
			}
			else{// recovery the pervious state 
				if (stack <= 0){
					actor.SetCurrentAction(NULL, 0, IdleID);
				}
				else{
					actor.SetCurrentAction(NULL, 0, RunID);
				}
			}
		}
	}
	else if (CurPoseID == UltimateAttackID){
		isNPCHitUltimate();
	
		if (!actor.Play(ONCE, (float)skip, FALSE, TRUE, TRUE)){
			if (stack <= 0){
				actor.SetCurrentAction(NULL, 0, IdleID);
			}
			else{
				actor.SetCurrentAction(NULL, 0, RunID);
			}
		}

	}
	else if (CurPoseID == GuardID){
		actor.Play(LOOP, (float)skip, FALSE, TRUE, TRUE);
	}
	else if (CurPoseID == RightDamageID)
	{
		if (!actor.Play(ONCE, (float)skip, FALSE, TRUE, TRUE)){
			// recovery the pervious state 
			if (stack <= 0){
				actor.SetCurrentAction(NULL, 0, IdleID);
			}
			else{
				actor.SetCurrentAction(NULL, 0, RunID);
			}
		}
	}
	else if (CurPoseID == DieID){

		actor.Play(ONCE, (float)skip, FALSE, TRUE, TRUE);
	}

	// special skill effect
	// count how many step left, rotate camera and show slash effect
	CurPoseID = actor.GetCurrentAction(NULL);
	FnObject fcobj;
	fcobj.ID(fcid);
	if (CurPoseID == UltimateAttackID){
		float rotate = 0;
		fcobj.Show(TRUE);
		skill_camera_timer += 1;
		rotate = skill_camera_timer * 2 * pi / (120 - 30);
		if (rotate > 2 * pi) rotate = 2 * pi;
		//sprintf(debugbuf, "skill t %f", skill_camera_timer);
		//camera rotate
		Camera3PersonView(rotate);
		CameraCollision();
		//slash effect
		if (skill_camera_timer > 80){
			FnGameFXSystem gxS(lyfxID);
			FnObject actorWeapon;
			//float pos[3];
			//actor.GetPosition(pos);
			//gxS.SetPlayLocation(pos);
			gxS.SetParentObjectForAll(lyfxdumID);
			gxS.Play((float)skip, ONCE);
		}
	}
	else {
		fcobj.Show(FALSE);
	}





	//==============================================
	//					NPC ai 	
	//==============================================
	for (int i = 1; i <= NPC_NUMBER; i++){
		if (npc[i].HealthPoints != 0){
			// movement
			FnCharacter npcChar;
			float curFDir[3];
			float pos[3], fDir[3], uDir[3], apos[3];
			
			npcChar.ID(npc[i].ID);
			npcChar.GetPosition(pos);
			npcChar.GetDirection(curFDir, NULL);
			actor.GetPosition(apos);
			vector<Node*>::iterator it;
			Node *Goal = NPCmovement(i);	// No.i npc's movement
			Node *temp, *pre;
			temp = Goal;
			pre = Goal->parent;
			while (true){
				if (pre->pos[0] == pos[0] && pre->pos[1] == pos[1] && pre->pos[2] == pos[2]){
					if (temp->pos[0] != pos[0] || temp->pos[1] != pos[1] || temp->pos[2] != pos[2]){
						fDir[0] = temp->pos[0] - pos[0]; fDir[1] = temp->pos[1] - pos[1]; fDir[2] = temp->pos[2] - pos[2];
						//fDir[0] = 0.8*curFDir[0] + 0.2*fDir[0]; fDir[1] = 0.8*curFDir[1] + 0.2*fDir[1]; fDir[2] = 0.8*curFDir[2] + 0.2*fDir[2];
						uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
						//npcChar.SetPosition(temp->pos);
						npcChar.SetDirection(fDir, uDir);
						npcChar.MoveForward(npc[i].movespeed, TRUE, FALSE, 0.0f, TRUE);
						npc[i].running = true;
						npc[i].canAttack = false;

						float *tem = (float *)malloc(3 * sizeof(float));
						tem[0] = pos[0];
						tem[1] = pos[1];
						tem[2] = pos[2];

						npc[i].pre_pos.push_back(tem);
						if (npc[i].pre_pos.size() > 30){
							free(npc[i].pre_pos.front());
							npc[i].pre_pos.pop_front();
						}
					}
					else{
						fDir[0] = apos[0] - pos[0]; fDir[1] = apos[1] - pos[1]; fDir[2] = apos[2] - pos[2];
						uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
						npcChar.SetDirection(fDir, uDir);
						npc[i].running = false;
						npc[i].canAttack = true;
					}
					for (it = nodelist.begin(); it != nodelist.begin() + 1; it++){
						free((*it));
					}
					open.clear();
					close.clear();
					nodelist.clear();
					break;
				}
				else{
					temp = pre;
					pre = pre->parent;
				}
			}

			//attack cool down
			npc[i].attack_counter++;
			npc[i].attack_counter %= (int)(30 / npc[i].attackrate);
			

		}
	}


	for (int i = 1; i <= NPC_NUMBER; i++){
		FnCharacter npcChar;
		ACTIONid npcCurPoseID;
		npcChar.ID(npc[i].ID);
		npcCurPoseID = npcChar.GetCurrentAction(NULL, 0);
		
		if (npcCurPoseID == npc[i].IdleID)
		{
			npcChar.Play(LOOP, (float)skip, FALSE, TRUE);
			if (npc[i].running){
				npcChar.SetCurrentAction(NULL, 0, npc[i].RunID);
			}
			if (npc[i].canAttack){
				if (npc[i].attack_counter == 50){
					npcChar.SetCurrentAction(NULL, 0, npc[i].NormalAttack1ID);
					actor_AlreadyHit = false;
				}
				if (npc[i].attack_counter == 0){
					npcChar.SetCurrentAction(NULL, 0, npc[i].HeavyAttack1ID);
					actor_AlreadyHit = false;
				}
			}
		}
		else if (npcCurPoseID == npc[i].RunID)
		{
			npcChar.Play(LOOP, (float)skip, FALSE, TRUE);
			if (!npc[i].running){
				npcChar.SetCurrentAction(NULL, 0, npc[i].IdleID);
			}
		}
		else if (npcCurPoseID == npc[i].DieID)
		{
			npcChar.Play(ONCE, (float)skip, FALSE, TRUE);
			npcChar.Play(ONCE, (float)skip, FALSE, TRUE);
		}
		else if (npcCurPoseID == npc[i].NormalAttack1ID)
		{
			if (npc[i].attackType.compare("melee")==0)
				NPCattackActor(i);
			npcChar.Play(ONCE, (float)skip, FALSE, TRUE);
			if (!npcChar.Play(ONCE, (float)skip, FALSE, TRUE))
			{
				if (npc[i].attackType.compare("range") == 0)
					NPCAttackActorRange(i);
				npcChar.SetCurrentAction(NULL, 0, npc[i].IdleID);
			}
		}
		else if (npcCurPoseID == npc[i].HeavyAttack1ID)
		{
			if (npc[i].attackType.compare("melee") == 0)
				NPCattackActor(i);
			npcChar.Play(ONCE, (float)skip, FALSE, TRUE);
			if (!npcChar.Play(ONCE, (float)skip, FALSE, TRUE))
			{
				if (npc[i].attackType.compare("range") == 0)
					NPCAttackActorRange(i);
				npcChar.SetCurrentAction(NULL, 0, npc[i].IdleID);
			}
		}
		else if (npcCurPoseID == npc[i].Damage1ID)
		{
			if (!npcChar.Play(ONCE, (float)skip, FALSE, TRUE))
			{
				npcChar.SetCurrentAction(NULL, 0, npc[i].IdleID);
			}
		}

	}

	// move logic and 3 person view camera
	if (CurPoseID == RunID){
		if (FyCheckHotKeyStatus(FY_UP)) {
			actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
			Camera3PersonView(0);
		}

		if (FyCheckHotKeyStatus(FY_LEFT)) {
			actor.TurnRight(-rotate);
			Camera3PersonView(0);
		}

		if (FyCheckHotKeyStatus(FY_RIGHT)) {
			actor.TurnRight(rotate);
			Camera3PersonView(0);
		}

		if (FyCheckHotKeyStatus(FY_DOWN)) {
			actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
			Camera3PersonView(pi);
		}
		CameraCollision();
	}
	// idle logic
	if ( CurPoseID == IdleID){}
}

void Camera3PersonView(float rotate)
{
	// camera
	float apos[3], cpos[3]; //actor,camera position
	float fDir[3], uDir[3], rDir[3]; //actor face, up, right dir;
	float cfDir[3], cuDir[3]; // camera face, up dir
	float distance = 500, height = 100;
	FnCharacter actor;
	FnCamera camera;
	actor.ID(actorID);
	camera.ID(cID);
	// get actor position & direction
	actor.GetPosition(apos);
	actor.GetDirection(fDir, uDir);
	// calculate camera position & direction
	// camera pos
	cpos[0] = apos[0] - (fDir[0] * cos(rotate) - fDir[1] * sin(rotate)) * distance;
	cpos[1] = apos[1] - (fDir[0] * sin(rotate) + fDir[1] * cos(rotate)) * distance;
	cpos[2] = apos[2] + uDir[2] * height;
	// camera face
	cfDir[0] = apos[0] - cpos[0];
	cfDir[1] = apos[1] - cpos[1];
	cfDir[2] = apos[2] - cpos[2] + height / 2;
	// camera up
	cross3(rDir, cfDir, uDir);
	cross3(cuDir, rDir, cfDir);
	// camera set
	camera.SetPosition(cpos);
	camera.SetDirection(cfDir, cuDir);
}

// collision cam modification
void CameraCollision(){
	// collision check
	float apos[3], cpos[3]; //actor,camera position
	float fDir[3], uDir[3]; //actor face, up, right dir;
	float rDir[3];
	float cfDir[3], cuDir[3]; // camera face, up dir
	float distance = 500, height = 100;
	float hitray[3], hitpos[3]; // collision ray and position
	FnCharacter actor;
	FnCamera camera;
	FnObject terrain;
	actor.ID(actorID);
	camera.ID(cID);
	terrain.ID(tID);
	// get actor position & direction
	actor.GetPosition(apos);
	actor.GetDirection(fDir, uDir);
	// get camera pos & dir
	camera.GetPosition(cpos);
	camera.GetDirection(cfDir, cuDir);
	// hitray from actor to camera
	hitray[0] = cpos[0] - apos[0];
	hitray[1] = cpos[1] - apos[1];
	hitray[2] = cpos[2] - apos[2];
	if (terrain.HitTest(apos, hitray, hitpos) > 0){
		// check distance of camera and hitpoint to camera
		if (dist3(apos, hitpos)<dist3(apos, cpos)){
			cpos[0] = hitpos[0];
			cpos[1] = hitpos[1];
			cpos[2] = apos[2] + sqrt(distance*distance + height*height - (apos[0] - cpos[0])*(apos[0] - cpos[0]) - (apos[1] - cpos[1])*(apos[1] - cpos[1]));
		}
		// recount raised camera position
	}
	// camera face
	cfDir[0] = apos[0] - cpos[0];
	cfDir[1] = apos[1] - cpos[1];
	cfDir[2] = apos[2] - cpos[2] + height / 2;
	// camera up
	cross3(rDir, cfDir, uDir);
	cross3(cuDir, rDir, cfDir);
	// rewind down ward
	if (cuDir[2] < 0){
		cross3(rDir, uDir, fDir);
		cross3(cuDir, cfDir, rDir);
	}
	// camera set
	camera.SetPosition(cpos);
	camera.SetDirection(cfDir, cuDir);
}

/*----------------------
perform the rendering
C.Wang 0720, 2006
-----------------------*/
void RenderIt(int skip)
{
	FnViewport vp;

	// render the whole scene
	vp.ID(vID);
	vp.Render3D(cID, TRUE, TRUE);
	

	// get camera's data
	FnCamera camera;
	camera.ID(cID);
	

	float pos[3], fDir[3], uDir[3];
	camera.GetPosition(pos);
	camera.GetDirection(fDir, uDir);


	//get actor's data
	FnCharacter actor;
	actor.ID(actorID);
	FnObject weapon;
	weapon.ID(actor.GetBoneObject("WeaponDummy03"));

	float actorPos[3], weaponPos[3];
	actor.GetPosition(actorPos);
	weapon.GetPosition(weaponPos);

	// show frame rate
	static char string[128];
	if (frame == 0) {
		FyTimerReset(0);
	}

	if (frame / 10 * 10 == frame) {
		float curTime;

		curTime = FyTimerCheckTime(0);
		sprintf(string, "Fps: %6.2f", frame / curTime);
	}

	frame += skip;
	if (frame >= 1000) {
		frame = 0;
	}
	//sprintf(debugbuf,"frame %d", frame);
	if (audobj.IsPlaying()){
		sprintf(debugbuf, "play");
	}
	else if (!(audobj.IsPlaying())){
		sprintf(debugbuf, "no play");
	}

	FnText text;
	text.ID(textID);

	text.Begin(vID);
	text.Write(string, 20, 20, 255, 0, 0);

	char posS[256], fDirS[256], uDirS[256];
	sprintf(posS, "pos: %8.3f %8.3f %8.3f", pos[0], pos[1], pos[2]);
	sprintf(fDirS, "facing: %8.3f %8.3f %8.3f", fDir[0], fDir[1], fDir[2]);
	sprintf(uDirS, "up: %8.3f %8.3f %8.3f", uDir[0], uDir[1], uDir[2]);

	text.Write(posS, 20, 35, 255, 255, 0);
	text.Write(fDirS, 20, 50, 255, 255, 0);
	text.Write(uDirS, 20, 65, 255, 255, 0);

	char actorPosS[256], weaponPosS[256], npctatus[256];
	sprintf(actorPosS, "actor pos: %8.3f %8.3f %8.3f", actorPos[0], actorPos[1], actorPos[2]);
	sprintf(weaponPosS, "weapon pos: %d %8.3f %8.3f", nodelist.size(), weaponPos[1], weaponPos[2]);
	sprintf(npctatus, "NPC1 Life: %d  /  NPC2 Life: %d  / actor Life: %d", projectiles.size(), npc[2].HealthPoints, actor_HealthPoints);


	text.Write(actorPosS, 20, 80, 255, 255, 0);
	text.Write(weaponPosS, 20, 100, 255, 255, 0);
	text.Write(npctatus, 20, 120, 255, 255, 0);
	text.Write(debugbuf, 20, 140, 255, 255, 0);
	text.End();




	// the lyubu hpID
	FnObject actor_hpobj;
	actor_hpobj.ID(actor_hpid);
	actorPos[2] = actorPos[2] + 95;
	actor_hpobj.SetPosition(actorPos);
	
	// the npc hpID
	for (int i = 1; i <= NPC_NUMBER; i++){
		FnCharacter npcChar;
		FnObject npchpobj;
		float npcPos[3];

		npcChar.ID(npc[i].ID);
		npcChar.GetPosition(npcPos);
		npchpobj.ID(npc[i].hpid);
		
		if (i == 1){
			// the donzo hpID
			npcPos[2] = npcPos[2] + 100;
		}
		else if (i >= 2 && i<=3){
			// the Robber hpID
			npcPos[2] = npcPos[2] + 70;
		}
		else if (i >= 4 && i <= 6){
			npcPos[2] = npcPos[2] + 100;
		}
		
		npchpobj.SetPosition(npcPos);
	}	
	
	// focus special effect picture
	FnObject fcobj;
	float fcpos[3];
	float fcdis = 300;
	fcobj.ID(fcid);
	camera.ID(cID);
	camera.GetPosition(pos);
	camera.GetDirection(fDir, uDir);
	fcpos[0] = pos[0] + fDir[0] * fcdis;
	fcpos[1] = pos[1] + fDir[1] * fcdis;
	fcpos[2] = pos[2] + fDir[2] * fcdis;
	fcobj.SetPosition(fcpos);

	// swap buffer
	FySwapBuffers();
}

void StartMenu(int skip){
	// show menu background picture
	FnScene scene(sID);
	FnObject bgobj;
	OBJECTid bgID;
	float bgsize[2] = { 120, 90 };
	float bgpos[3];
	float bgdis = 270;
	bgID = scene.CreateObject(OBJECT);
	bgobj.ID(bgID);
	bgobj.Show(TRUE);
	bgobj.Billboard(NULL, bgsize, "Data\\NTU6\\Menu\\menu_background", 0);
	// abjust  position
	FnCamera camera(cID);
	float pos[3], fdir[3], cdir[3];
	camera.GetPosition(pos);
	camera.GetDirection(fdir, cdir);
	bgpos[0] = pos[0] + fdir[0] * bgdis;
	bgpos[1] = pos[1] + fdir[1] * bgdis;
	bgpos[2] = pos[2] + fdir[2] * bgdis;
	bgobj.SetPosition(bgpos);
	// show menu start word
	FnObject swobj;
	OBJECTid swID;
	float swsize[2] = { 120, 90 };
	float swpos[3];
	float swdis = 250;
	swID = scene.CreateObject(OBJECT);
	swobj.ID(swID);
	swobj.SetAlphaFlag(TRUE);
	swobj.Show(TRUE);
	swobj.Billboard(NULL, swsize, "Data\\NTU6\\Menu\\start", 0);
	// abjust  position
	camera.GetPosition(pos);
	camera.GetDirection(fdir, cdir);
	swpos[0] = pos[0] + fdir[0] * swdis;
	swpos[1] = pos[1] + fdir[1] * swdis;
	swpos[2] = pos[2] + fdir[2] * swdis;
	swobj.SetPosition(swpos);

	// render the whole scene
	FnViewport vp;
	vp.ID(vID);
	vp.Render3D(cID, TRUE, TRUE);
	// swap buffer
	FySwapBuffers();

	// key detect
	if (FyCheckHotKeyStatus(FY_E)){
		// start from menu
		FyBindTimer(5, 0.0f, NULL, TRUE);
		// bind main game timers, frame rate = 30 fps
		FyBindTimer(0, 30.0f, GameAI, TRUE);
		FyBindTimer(1, 30.0f, RenderIt, TRUE);
		FyBindTimer(2, 30.0f, projectileUpdate, TRUE);
	}

	// clear menu
	scene.DeleteObject(bgID);
	// clear word
	scene.DeleteObject(swID);
}

OBJECTid ewID;
void init_EndMenu(){
	FnScene scene(sID);
	ewID = scene.CreateObject(OBJECT);
}

void EndMenu(int skip){

	FnCamera camera(cID);
	float pos[3], fdir[3], cdir[3];
	// show menu end word
	FnObject ewobj;
	float ewsize[2] = { 120, 90 };
	float ewpos[3];
	float ewdis = 250;
	ewobj.ID(ewID);
	ewobj.SetAlphaFlag(TRUE);
	ewobj.Show(TRUE);
	ewobj.Billboard(NULL, ewsize, "Data\\NTU6\\Menu\\end", 0);
	// abjust  position
	camera.GetPosition(pos);
	camera.GetDirection(fdir, cdir);
	ewpos[0] = pos[0] + fdir[0] * ewdis;
	ewpos[1] = pos[1] + fdir[1] * ewdis;
	ewpos[2] = pos[2] + fdir[2] * ewdis;
	ewobj.SetPosition(ewpos);
	// key detect
	if (FyCheckHotKeyStatus(FY_E)){
		FyQuitFlyWin32();
	}
}

void EndKey(BYTE code, BOOL4 value)
{
	if (value && not_end == 1) {
		not_end = 0;
		// init end menu and keep show it in camera
		init_EndMenu();
		FyBindTimer(5, 60.0f, EndMenu, TRUE);
	}
}

void projectileUpdate(int skip)
{
	vector<Projectile *>::iterator it;
	float apos[3],min[3], max[3];
	FnCharacter actor;
	bool hitActor;

	actor.ID(actorID);
	actor.GetPosition(apos); 
	min[0] = apos[0] - 30;
	min[1] = apos[1] - 30;
	min[2] = apos[2];

	max[0] = apos[0] + 30;
	max[1] = apos[1] + 30;
	max[2] = apos[2] + 50;
	for (it = projectiles.begin(); it != projectiles.end(); it++){
		(*it)->fly(skip);
		hitActor = (*it)->hitTest(min, max);
		if ((*it)->isHit() || (*it)->island()){
			//free((*it));
			//projectiles.erase(it);
		}
		if (hitActor)
		{
			FnBillboard actor_hpboard;
			actor_hpboard.ID(actor_hpboardid);
			float actor_hpsize[2] = { 50, 5 };
			//sound
			FnAudio actorishit_sound;

			CurPoseID = actor.GetCurrentAction(NULL, 0);
			if ( !actor_AlreadyHit && CurPoseID != DieID && CurPoseID == GuardID){
				float afDir[3], rDir[3], rAngle;
				float* pStart;
				pStart = (*it)->getStart();
				actor.GetDirection(afDir, NULL);
				rDir[0] = pStart[0] - apos[0]; rDir[1] = pStart[1] - apos[1]; rDir[2] = pStart[2] - apos[2];
				rAngle = acosf((rDir[0] * afDir[0] + rDir[1] * afDir[1] + rDir[2] * afDir[2])
								/ (sqrt(rDir[0] * rDir[0] + rDir[1] * rDir[1] + rDir[2] * rDir[2])
									+ sqrt(afDir[0] * afDir[0] + afDir[1] * afDir[1] + afDir[2] * afDir[2])));

				if (abs(rAngle) < 1.5){
					// not hurt
					return;
				}
				else{
					actor_AlreadyHit = true;
					actor_HealthPoints -= 5;

					playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_behit", 0.5f);

					//hp 's picture is shorter
					actor_hpsize[0] = actor_hpsize[0] * actor_HealthPoints / 100;
					actor_hpboard.SetPositionSize(NULL, actor_hpsize);

					if (actor_HealthPoints <= 0)
					{
						actor.SetCurrentAction(NULL, 0, DieID);
						playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_die", 1);
					}
					else if (CurPoseID == GuardID){
						actor.SetCurrentAction(NULL, 0, RightDamageID);
					}
				}
			}
			if ( !actor_AlreadyHit && CurPoseID != DieID && CurPoseID != GuardID)
			{
				actor_AlreadyHit = true;
				actor_HealthPoints -= 5;

				//sound
				playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_behit", 0.5f);

				//hp 's picture is shorter
				actor_hpsize[0] = actor_hpsize[0] * actor_HealthPoints / 100;
				actor_hpboard.SetPositionSize(NULL, actor_hpsize);

				if (actor_HealthPoints <= 0)
				{
					actor.SetCurrentAction(NULL, 0, DieID);
					playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_die", 1);
				}
				else if (CurPoseID == IdleID){
					actor.SetCurrentAction(NULL, 0, RightDamageID);
				}

			}
		}
	}
}

/*------------------
movement control
-------------------*/

void Movement(BYTE code, BOOL4 value)
{
	//===============================
	//		Homework #01 part 2
	//===============================
	FnCharacter actor;
	actor.ID(actorID);
	if (!value) {
		stack--;
		if (CurPoseID != RunID && CurPoseID != IdleID) return;//fix bug for release move hotkey interruput attack
		if (stack <= 0) {
			CurPoseID = IdleID;
			actor.SetCurrentAction(NULL, 0, CurPoseID);
			actor.Play(START, 0.0f, FALSE, TRUE);
		}
	}
	else {
		stack++;
		if (CurPoseID != RunID && CurPoseID != IdleID) return;//fix bug for release move hotkey interruput attack

		CurPoseID = RunID;
		actor.SetCurrentAction(NULL, 0, CurPoseID);
		actor.Play(START, 0.0f, FALSE, TRUE);

		if (code == FY_DOWN){
			float fDir[3], uDir[3];
			actor.GetDirection(fDir, uDir);
			fDir[0] = -fDir[0];
			fDir[1] = -fDir[1];
			actor.SetDirection(fDir, uDir);
		}

	}
}

Node* NPCmovement(int number)
{
	// A* algorithm
	int counter = 0;
	FnCharacter actor;
	float apos[3];
	actor.ID(actorID);
	actor.GetPosition(apos);
	Node *start;
	start = (struct Node *)malloc(sizeof(struct Node));
	nodelist.push_back(start);
	FnCharacter npcChar;
	npcChar.ID(npc[number].ID);
	npcChar.GetPosition(start->pos);
	start->parent = start;
	start->gn = 0;
	start->hn = abs(start->pos[0] - apos[0]) + abs(start->pos[1] - apos[1]) + abs(start->pos[2] - apos[2]);
	start->fn = start->gn + start->hn;
	open.push_back(start);
	while (!open.empty()){
		vector<Node*>::iterator it, minit;

		// get node n in open which has lowest fn
		Node* n;
		float minfn = 200000;
		counter++;
		for (it = open.begin(); it != open.end(); it++){
			if ((*it)->fn < minfn){
				minfn = (*it)->fn;
				minit = it;
				n = (*it);
			}
		}

		if (sqrt(dist3(n->pos, apos)) < npc[number].attack_range || counter >= 6){
			return n;
		}
		open.erase(minit);
		close.push_back(n);

		// for each n' = CanMove(n, direction)
		FnObject terrain;
		terrain.ID(tID);
		float hitray[3], hitpos[3]; // collision ray and position
		minfn = 200000;
		for (int i = 0; i < 4; i++){
			Node *neighbor = (struct Node *)malloc(sizeof(struct Node));
			float fDir[3], uDir[3];
			if (i == 0){	//east
				neighbor->pos[0] = n->pos[0] + npc[number].movespeed; neighbor->pos[1] = n->pos[1]; neighbor->pos[2] = n->pos[2];
			}
			else if (i == 1){	//west
				neighbor->pos[0] = n->pos[0] - npc[number].movespeed; neighbor->pos[1] = n->pos[1]; neighbor->pos[2] = n->pos[2];
			}
			else if (i == 2){	//south
				neighbor->pos[0] = n->pos[0]; neighbor->pos[1] = n->pos[1] - npc[number].movespeed; neighbor->pos[2] = n->pos[2];
			}
			else if (i == 3){	//north
				neighbor->pos[0] = n->pos[0]; neighbor->pos[1] = n->pos[1] + npc[number].movespeed; neighbor->pos[2] = n->pos[2];
			}

			neighbor->gn = n->gn + npc[number].movespeed;
			neighbor->hn = abs(neighbor->pos[0] - apos[0]) + abs(neighbor->pos[1] - apos[1]) + abs(neighbor->pos[2] - apos[2]);
			neighbor->fn = neighbor->gn + neighbor->hn;

			

			// check collision
			hitray[0] = neighbor->pos[0] - n->pos[0];
			hitray[1] = neighbor->pos[1] - n->pos[1];
			hitray[2] = neighbor->pos[2] - n->pos[2];

			
			// set Kagemusha's position and face
			fDir[0] = neighbor->pos[0] - n->pos[0]; fDir[1] = neighbor->pos[1] - n->pos[1]; fDir[2] = neighbor->pos[2] - n->pos[2];
			uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
			Kagemusha.SetPosition(n->pos);
			Kagemusha.SetDirection(fDir, uDir);
			
			if (WALK != Kagemusha.MoveForward(npc[i].movespeed, TRUE, FALSE, 0.0f, TRUE)){
				// collide to wall
				free(neighbor);
				continue;
			}
			
			/*
			if (terrain.HitTest(n->pos, hitray, hitpos) > 0){
				if (sqrt(dist3(n->pos, hitpos)) < 500+sqrt(dist3(n->pos, neighbor->pos))){
					// collide to wall
					free(neighbor);
					continue;
				}
			}
			*/
			if (NPCcollideToOther(number, neighbor->pos))
			{
				free(neighbor);
				continue;
			}

			bool xflag = false;
			list<float*>::iterator quit;
			for (int i = 1; i <= NPC_NUMBER; i++){
				if (number == i){	
					// check its pre_pos, don't step on that
					for (quit = npc[i].pre_pos.begin(); quit != npc[i].pre_pos.end(); quit++){
						if (dist3(neighbor->pos, (*quit)) < 7){
							free(neighbor);
							xflag = true;
							continue;
						}
					}
					break;
				}
				else{
					continue;
				}
			}
			if (xflag)
				continue;
			

			// check OPEN & CLOSED list
			// if n' in OPEN list and not better, continue
			bool flag = false;
			vector<Node*>::iterator is;
			for (it = open.begin(); it != open.end(); it++){
				if (neighbor->pos[0] == (*it)->pos[0]
					&& neighbor->pos[1] == (*it)->pos[1]
					&& neighbor->pos[2] == (*it)->pos[2]){
					if (neighbor->fn >= (*it)->fn){
						flag = true;
					}
					break;
				}
			}
			// if n' in CLOSED list and not better, continue
			for (is = close.begin(); is != close.end(); is++){
				if (neighbor->pos[0] == (*is)->pos[0]
					&& neighbor->pos[1] == (*is)->pos[1]
					&& neighbor->pos[2] == (*is)->pos[2]){
					if (neighbor->fn >= (*is)->fn){
						flag = true;
					}
					break;
				}
			}
			if (flag){
				free(neighbor);
				continue;
			}
			else{
				//remove any n' from OPEN and CLOSED
				if (it != open.end()){
					open.erase(it);
				}
				if (is != close.end()){
					close.erase(is);
				}
			}

			neighbor->parent = n;
			open.push_back(neighbor);	
			nodelist.push_back(neighbor);
		}

	}

	return start;


}


void ActorAttack(BYTE code, BOOL4 value)
{
	if (!value) return;
	for (int i = 1; i <= NPC_NUMBER; i++){
		npc[i].AlreadyBeenHit = false;
	}
	
	FnCharacter actor;
	actor.ID(actorID);
	CurPoseID = actor.GetCurrentAction(NULL, 0);
	if (code == FY_Z){
		if (CurPoseID == IdleID || CurPoseID == RunID){
			actor.SetCurrentAction(NULL, 0, NormalAttack1ID);
			isCombo = false;
			NextAttackID = NormalAttack2ID;
		}
		else if (CurPoseID == NormalAttack1ID){
			isCombo = true;
			NextAttackID = NormalAttack2ID;
		}
		else if (CurPoseID == NormalAttack2ID){
			isCombo = true;
			NextAttackID = NormalAttack3ID;
		}
		else if (CurPoseID == NormalAttack3ID){
			isCombo = true;
			NextAttackID = NormalAttack4ID;
		}
		else if (CurPoseID == NormalAttack4ID){
			isCombo = true;
			NextAttackID = NormalAttack1ID;
		}
	}
	else if (code == FY_X){
		if (CurPoseID == IdleID || CurPoseID == RunID){
			actor.SetCurrentAction(NULL, 0, HeavyAttack1ID);
			isCombo = false;
			NextAttackID = HeavyAttack2ID;
		}
		else if (CurPoseID == HeavyAttack1ID){
			isCombo = true;
			NextAttackID = HeavyAttack2ID;
		}
		else if (CurPoseID == HeavyAttack2ID){
			isCombo = true;
			NextAttackID = HeavyAttack3ID;
		}
		else if (CurPoseID == HeavyAttack3ID){
			isCombo = true;
			NextAttackID = HeavyAttack1ID;
		}
	}
	else if (code == FY_C){
		if (CurPoseID == IdleID || CurPoseID == RunID){
			actor.SetCurrentAction(NULL, 0, UltimateAttackID);
			// special effect setup
			skill_camera_timer = 0;
			FnGameFXSystem gfx(lyfxID);
			FnObject dummy(lyfxdumID);
			float pos[3],fdir[3],udir[3];
			actor.GetPosition(pos);
			actor.GetDirection(fdir, udir);
			gfx.Load("mod_Lyubu_atk01", TRUE);
			dummy.SetPosition(pos);
			// force rotate slash fx to match last hit in ultimate attack 
			float ddir[3];
			cross3(ddir, fdir, udir);
			udir[0] += ddir[0] * 1;
			udir[1] += ddir[1] * 1;
			cross3(ddir, udir, fdir);
			fdir[0] += ddir[0] * 1;
			fdir[1] += ddir[1] * 1;
			fdir[2] += ddir[2] * 1;
			dummy.SetDirection(fdir, udir);

			
			// sound

			if (!audobj.IsPlaying()){
				audobj.ID(FyCreateAudio());
				audobj.Load("Data\\NTU6\\Media\\lyubu_ultimate");
				audobj.SetVolume(200.0f);
				audobj.Play(ONCE);

			}
			
			
		}
		
	}
}

void ActorDefence(BYTE code, BOOL4 value)
{
	FnCharacter actor;
	actor.ID(actorID);
	CurPoseID = actor.GetCurrentAction(NULL, 0);
	if (CurPoseID != DieID && CurPoseID != RightDamageID){
		if (value){
			if (code == FY_SPACE){
				actor.SetCurrentAction(NULL, 0, GuardID);
			}
		}
		else{
			actor.SetCurrentAction(NULL, 0, IdleID);
		}
	}
}

/*------------------
quit the demo
C.Wang 0327, 2005
-------------------*/
void QuitGame(BYTE code, BOOL4 value)
{
	if (code == FY_ESCAPE) {
		if (value) {
			FyQuitFlyWin32();
		}
	}
}



/*-----------------------------------
initialize the pivot of the camera
C.Wang 0329, 2005
------------------------------------*/
void InitPivot(int x, int y)
{
	oldX = x;
	oldY = y;
	frame = 0;
}


/*------------------
pivot the camera
C.Wang 0329, 2005
-------------------*/
void PivotCam(int x, int y)
{
	FnObject model;

	if (x != oldX) {
		model.ID(cID);
		model.Rotate(Z_AXIS, (float)(x - oldX), GLOBAL);
		oldX = x;
	}

	if (y != oldY) {
		model.ID(cID);
		model.Rotate(X_AXIS, (float)(y - oldY), GLOBAL);
		oldY = y;
	}
}


/*----------------------------------
initialize the move of the camera
C.Wang 0329, 2005
-----------------------------------*/
void InitMove(int x, int y)
{
	oldXM = x;
	oldYM = y;
	frame = 0;
}


/*------------------
move the camera
C.Wang 0329, 2005
-------------------*/
void MoveCam(int x, int y)
{
	if (x != oldXM) {
		FnObject model;

		model.ID(cID);
		model.Translate((float)(x - oldXM)*2.0f, 0.0f, 0.0f, LOCAL);
		oldXM = x;
	}
	if (y != oldYM) {
		FnObject model;

		model.ID(cID);
		model.Translate(0.0f, (float)(oldYM - y)*2.0f, 0.0f, LOCAL);
		oldYM = y;
	}
}


/*----------------------------------
initialize the zoom of the camera
C.Wang 0329, 2005
-----------------------------------*/
void InitZoom(int x, int y)
{
	oldXMM = x;
	oldYMM = y;
	frame = 0;
}


/*------------------
zoom the camera
C.Wang 0329, 2005
-------------------*/
void ZoomCam(int x, int y)
{
	if (x != oldXMM || y != oldYMM) {
		FnObject model;

		model.ID(cID);
		model.Translate(0.0f, 0.0f, (float)(x - oldXMM)*10.0f, LOCAL);
		oldXMM = x;
		oldYMM = y;
	}
}

int NPCcollideToOther(int number, float *pos)
{	
	for (int i = 1; i <= NPC_NUMBER; i++){
		if (number == i)
			continue;
		
		FnCharacter otherNpc;
		ACTIONid CurPoseID;
		float otherNpc_pos[3];
		float otherNpc_corner1[3], otherNpc_corner2[3], otherNpc_corner3[3], otherNpc_corner4[3];
		otherNpc.ID(npc[i].ID);
		otherNpc.GetPosition(otherNpc_pos);
		CurPoseID = otherNpc.GetCurrentAction(NULL, 0);
		
		if (CurPoseID == npc[i].DieID)
			continue;

		// other NPC's box
		otherNpc_corner1[0] = otherNpc_pos[0] + npc[i].box_size;
		otherNpc_corner1[1] = otherNpc_pos[1] + npc[i].box_size;
		otherNpc_corner2[0] = otherNpc_pos[0] - npc[i].box_size;
		otherNpc_corner2[1] = otherNpc_pos[1] + npc[i].box_size;
		otherNpc_corner3[0] = otherNpc_pos[0] + npc[i].box_size;
		otherNpc_corner3[1] = otherNpc_pos[1] - npc[i].box_size;
		otherNpc_corner4[0] = otherNpc_pos[0] - npc[i].box_size;
		otherNpc_corner4[1] = otherNpc_pos[1] - npc[i].box_size;
		
		if ((otherNpc_corner1[0] <= pos[0] + npc[i].box_size && otherNpc_corner1[0] >= pos[0] - npc[i].box_size
			&& otherNpc_corner1[1] <= pos[1] + npc[i].box_size && otherNpc_corner1[1] >= pos[1] - npc[i].box_size) ||
			(otherNpc_corner2[0] <= pos[0] + npc[i].box_size && otherNpc_corner2[0] >= pos[0] - npc[i].box_size
			&& otherNpc_corner2[1] <= pos[1] + npc[i].box_size && otherNpc_corner2[1] >= pos[1] - npc[i].box_size) ||
			(otherNpc_corner3[0] <= pos[0] + npc[i].box_size && otherNpc_corner3[0] >= pos[0] - npc[i].box_size
			&& otherNpc_corner3[1] <= pos[1] + npc[i].box_size && otherNpc_corner3[1] >= pos[1] - npc[i].box_size) ||
			(otherNpc_corner4[0] <= pos[0] + npc[i].box_size && otherNpc_corner4[0] >= pos[0] - npc[i].box_size
			&& otherNpc_corner4[1] <= pos[1] + npc[i].box_size && otherNpc_corner4[1] >= pos[1] - npc[i].box_size))
		{
			return 1;
		}
	}

	return 0;
}




void NPCattackActor(int number)
{
	FnCharacter actor, npcChar;
	FnObject npcWeapon, npcHand;
	float weaponPos[3], handPos[3], npcPos[3];
	float actorPos[3];
	float ray[3];
	float min[3];
	float max[3];

	actor.ID(actorID);
	npcChar.ID(npc[number].ID);

	npcWeapon.ID(npcChar.GetBoneObject("Weapon01"));
	npcWeapon.GetPosition(weaponPos);

	npcHand.ID(npcChar.GetBoneObject("Bip01_R_Hand"));
	npcHand.GetPosition(handPos);

	actor.GetPosition(actorPos);
	npcChar.GetPosition(npcPos);

	ray[0] = weaponPos[0] - handPos[0];
	ray[1] = weaponPos[1] - handPos[1];
	ray[2] = weaponPos[2] - handPos[2];

	min[0] = actorPos[0] - 30;
	min[1] = actorPos[1] - 30;
	min[2] = actorPos[2];

	max[0] = actorPos[0] + 30;
	max[1] = actorPos[1] + 30;
	max[2] = actorPos[2] + 50;

	FnBillboard actor_hpboard;
	actor_hpboard.ID(actor_hpboardid);
	float actor_hpsize[2] = { 50, 5 };
	//sound
	FnAudio actorishit_sound;

	CurPoseID = actor.GetCurrentAction(NULL, 0);
	if (rayTracer.isInterset(handPos, ray, 1, npc[number].attack_range/100, min, max) && !actor_AlreadyHit && CurPoseID != DieID && CurPoseID == GuardID){ 
		float afDir[3], rDir[3], rAngle;
		actor.GetDirection(afDir, NULL);
		rDir[0] = npcPos[0] - actorPos[0]; rDir[1] = npcPos[1] - actorPos[1]; rDir[2] = npcPos[2] - actorPos[2];
		rAngle = acosf((rDir[0] * afDir[0] + rDir[1] * afDir[1] + rDir[2] * afDir[2])
			/ (sqrt(rDir[0] * rDir[0] + rDir[1] * rDir[1] + rDir[2] * rDir[2])
			+ sqrt(afDir[0] * afDir[0] + afDir[1] * afDir[1] + afDir[2] * afDir[2])));

		if (abs(rAngle) < 1.5){
			// not hurt
			return;
		}
		else{
			actor_AlreadyHit = true;
			actor_HealthPoints -= 5;
			//sound
			playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_behit",0.5f);

			//hp 's picture is shorter
			actor_hpsize[0] = actor_hpsize[0] * actor_HealthPoints / 100;
			actor_hpboard.SetPositionSize(NULL, actor_hpsize);

			if (actor_HealthPoints <= 0)
			{
				actor.SetCurrentAction(NULL, 0, DieID);
				playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_die", 1);
			}
			else if (CurPoseID == GuardID){
				actor.SetCurrentAction(NULL, 0, RightDamageID);
			}
		}
	}
	if (rayTracer.isInterset(handPos, ray, 1, 1, min, max) && !actor_AlreadyHit && CurPoseID != DieID && CurPoseID != GuardID)
	{
		actor_AlreadyHit = true;
		actor_HealthPoints -= 5;

		//sound
		playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_behit",0.5f);
		
		//hp 's picture is shorter
		actor_hpsize[0] = actor_hpsize[0] * actor_HealthPoints / 100;
		actor_hpboard.SetPositionSize(NULL, actor_hpsize);

		if (actor_HealthPoints <= 0)
		{
			actor.SetCurrentAction(NULL, 0, DieID);
			playmusic(actorishit_sound, "Data\\NTU6\\Media\\lyubu_die", 1);

		}
		else if (CurPoseID == IdleID){
			actor.SetCurrentAction(NULL, 0, RightDamageID);
		}
		
	}
}

void NPCAttackActorRange(int number){
	FnCharacter actor, npcChar;
	FnObject npcWeapon, npcHand;
	FnScene scene;
	float npcPos[3],dir[3];
	float actorPos[3];
	float ray[3];

	scene.ID(sID);

	actor.ID(actorID);
	npcChar.ID(npc[number].ID);

	actor.GetPosition(actorPos);
	npcChar.GetPosition(npcPos);
	npcPos[2] += 30;

	dir[0] = actorPos[0] - npcPos[0];
	dir[1] = actorPos[1] - npcPos[1];
	dir[2] = 20;

	Projectile* projectile = new Projectile(dir, npc[number].attack_range, 20, npcPos, sID, npc[number].attack_special);
	projectiles.push_back(projectile);

}


void isNPCHitUltimate(){
	FnCharacter actor;
	FnCharacter npcChar;
	float apos[3];
	float pos[3];
	
	actor.ID(actorID);
	actor.GetPosition(apos);
	FnAudio npcishit_sound;
	
	//set npc die
	for (int i = 1; i <= NPC_NUMBER; i++){
		npcChar.ID(npc[i].ID);
		npcChar.GetPosition(pos);

		CurPoseID = npcChar.GetCurrentAction(NULL, 0);
		if (sqrt(dist3(pos, apos)) < 200 && ((int)skill_camera_timer % 5 == 0) && CurPoseID != npc[i].DieID){
			npcChar.SetCurrentAction(NULL, 0, npc[i].Damage1ID);
		}

		if (sqrt(dist3(pos, apos)) < 200 && !npc[i].AlreadyBeenHit && CurPoseID != npc[i].DieID && skill_camera_timer > 95){
			npc[i].AlreadyBeenHit = true;
			npc[i].HealthPoints = 0;
			npcChar.SetCurrentAction(NULL, 0, npc[i].DieID);

			//hp
			FnBillboard npc_hpboard;
			float npc_hpsize[2] = { 50, 5 };

			//hp 's picture is shorter
			npc_hpsize[0] = npc_hpsize[0] * npc[i].HealthPoints / 100;
			npc_hpboard.ID(npc[i].hpboardid);
			npc_hpboard.SetPositionSize(NULL, npc_hpsize);

			//sound
			playmusic(npcishit_sound, "Data\\NTU6\\Media\\lyubu_behit", 0.5f);

		}

	}
}

void isNPCHit()
{
	FnCharacter actor;
	FnCharacter npcChar;
	FnObject actorWeapon,actorHand;
	float weaponPos[3], handPos[3], actorPos[3];
	float npcPos[3];
	float ray[3];
	float min[3];
	float max[3];

	actor.ID(actorID);
	
	actorWeapon.ID(actor.GetBoneObject("WeaponDummy02"));
	actorWeapon.GetPosition(weaponPos);
	actorHand.ID(actor.GetBoneObject("Bip01_R_Hand"));
	actorHand.GetPosition(handPos);

	actor.GetPosition(actorPos);

	for (int i = 1; i <= NPC_NUMBER; i++){
		npcChar.ID(npc[i].ID);
		npcChar.GetPosition(npcPos);
		
		ray[0] = weaponPos[0] - handPos[0];
		ray[1] = weaponPos[1] - handPos[1];
		ray[2] = weaponPos[2] - handPos[2];

		min[0] = npcPos[0] - npc[i].box_size;
		min[1] = npcPos[1] - npc[i].box_size;
		min[2] = npcPos[2];

		max[0] = npcPos[0] + npc[i].box_size;
		max[1] = npcPos[1] + npc[i].box_size;
		max[2] = npcPos[2] + 50;

		//hp
		FnBillboard npc_hpboard;
		npc_hpboard.ID(npc[i].hpboardid);

		FnAudio npcishit_sound;

		float npc_hpsize[2] = { 50, 5 };
		CurPoseID = npcChar.GetCurrentAction(NULL, 0);

		if (rayTracer.isInterset(handPos, ray, 1, 1, min, max) && !npc[i].AlreadyBeenHit
			&& CurPoseID != npc[i].DieID)
		{
			npc[i].AlreadyBeenHit = true;
			npc[i].HealthPoints -= 20;

			//hp 's picture is shorter
			npc_hpsize[0] = npc_hpsize[0] * npc[i].HealthPoints / 100;
			npc_hpboard.SetPositionSize(NULL, npc_hpsize);
			
			//sound
			if (npc[i].HealthPoints <= 0)
			{
				npcChar.SetCurrentAction(NULL, 0, npc[i].DieID);
				if (i == 1){
					playmusic(npcishit_sound, "Data\\NTU6\\Media\\donzo_die",1.0f);
				}
				else if (i >= 2){
					playmusic(npcishit_sound, "Data\\NTU6\\Media\\robber_die",1.0f);

				}
			}
			else{
				npcChar.SetCurrentAction(NULL, 0, npc[i].Damage1ID);
			}

			//sound
			playmusic(npcishit_sound, "Data\\NTU6\\Media\\npc_behit",0.5f);

		}
	}
}

void playmusic(FnAudio music, char *filename,float v){


	music.ID(FyCreateAudio());
	music.Load(filename);
	music.SetVolume(v);
	music.Play(ONCE);

}
