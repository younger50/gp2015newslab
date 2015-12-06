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


VIEWPORTid vID;                 // the major viewport
SCENEid sID;                    // the 3D scene
OBJECTid cID, tID;              // the main camera and the terrain for terrain following
CHARACTERid actorID;            // the major character
int actorAttacking = 0, actorAttackFrame = 0; // actor global
CHARACTERid npc1ID, npc2ID;		// the npc character

// actor = lyubu
ACTIONid IdleID, RunID, WalkID, CurPoseID;
ACTIONid NormalAttack1ID, NormalAttack2ID, NormalAttack3ID, NormalAttack4ID; 
ACTIONid HeavyAttack1ID, HeavyAttack2ID, HeavyAttack3ID;
ACTIONid UltimateAttackID;
ACTIONid GuardID;
ACTIONid HeavyDamageID, RightDamageID, LeftDamageID, DieID;

unsigned int AttackRange = 100;
float AttackRadius = 1.0f;

bool isCombo = false;
ACTIONid NextAttackID;

void PlayActorAction(int skip); // play actor action frame by frame

void isNPCHit();


// npc1 = Donzo
ACTIONid npc1_IdleID, npc1_RunID, npc1_CurPoseID;
ACTIONid npc1_NormalAttack1ID, npc1_NormalAttack2ID; 
ACTIONid npc1_HeavyAttack1ID;
ACTIONid npc1_GuardID;
ACTIONid npc1_Damage1ID, npc1_Damage2ID, npc1_DieID;


// npc2 = robber
ACTIONid npc2_IdleID, npc2_RunID,npc2_CurPoseID;
ACTIONid npc2_NormalAttack1ID, npc2_NormalAttack2ID; 
ACTIONid npc2_HeavyAttack1ID;
ACTIONid npc2_Damage1ID, npc2_Damage2ID, npc2_DieID;

ROOMid terrainRoomID = FAILED_ID;
TEXTid textID = FAILED_ID;

// some globals
int frame = 0;
int oldX, oldY, oldXM, oldYM, oldXMM, oldYMM;

// hotkey callbacks
void QuitGame(BYTE, BOOL4);
void Movement(BYTE, BOOL4);
void ActorAttack(BYTE, BOOL4);


// timer callbacks
void GameAI(int);
void RenderIt(int);
void Camera3PersonView(int);

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
	BOOL4 beOK = FyStartFlyWin32("NTU@2015 Homework #02 - Use Fly2", 0, 0, 1024, 768, FALSE);

	// setup the data searching paths
	FySetShaderPath("Data\\NTU6\\Shaders");
	FySetModelPath("Data\\NTU6\\Scenes");
	FySetTexturePath("Data\\NTU6\\Scenes\\Textures");
	FySetScenePath("Data\\NTU6\\Scenes");

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
	npc1ID = scene.LoadCharacter("Donzo2");
	npc2ID = scene.LoadCharacter("Robber02");

	// put the character on terrain
	float pos[3], fDir[3], uDir[3];
	FnCharacter actor;
	actor.ID(actorID);
	pos[0] = 3569.0f; pos[1] = -3208.0f; pos[2] = 1000.0f;
	fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
	uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
	actor.SetDirection(fDir, uDir);
	actor.SetTerrainRoom(terrainRoomID, 10.0f);
	beOK = actor.PutOnTerrain(pos);

	float npc1_pos[3], npc1_fDir[3], npc1_uDir[3];
	FnCharacter npc1;
	npc1.ID(npc1ID);
	npc1_pos[0] = 3569.0f; npc1_pos[1] = -2908.0f; npc1_pos[2] = 1000.0f;
	npc1_fDir[0] = 1.0f; npc1_fDir[1] = 1.0f; npc1_fDir[2] = 0.0f;
	npc1_uDir[0] = 0.0f; npc1_uDir[1] = 0.0f; npc1_uDir[2] = 1.0f;
	npc1.SetDirection(npc1_fDir, npc1_uDir);
	npc1.SetTerrainRoom(terrainRoomID, 10.0f);
	beOK = npc1.PutOnTerrain(npc1_pos);


	float npc2_pos[3], npc2_fDir[3], npc2_uDir[3];
	FnCharacter npc2;
	npc2.ID(npc2ID);
	npc2_pos[0] = 3569.0f; npc2_pos[1] = -2608.0f; npc2_pos[2] = 1000.0f;
	npc2_fDir[0] = 1.0f; npc2_fDir[1] = 1.0f; npc2_fDir[2] = 0.0f;
	npc2_uDir[0] = 0.0f; npc2_uDir[1] = 0.0f; npc2_uDir[2] = 1.0f;
	npc2.SetDirection(npc2_fDir, npc2_uDir);
	npc2.SetTerrainRoom(terrainRoomID, 10.0f);
	beOK = npc2.PutOnTerrain(npc2_pos);

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

	npc1_IdleID = npc1.GetBodyAction(NULL, "Idle");
	npc1_RunID = npc1.GetBodyAction(NULL, "Run");
	npc1_NormalAttack1ID = npc1.GetBodyAction(NULL, "AttackL1");
	npc1_NormalAttack2ID = npc1.GetBodyAction(NULL, "AttackL2");
	npc1_HeavyAttack1ID = npc1.GetBodyAction(NULL, "Defence");
	npc1_GuardID = npc1.GetBodyAction(NULL, "HeavyDamage");
	npc1_Damage1ID = npc1.GetBodyAction(NULL, "DamageL");
	npc1_Damage2ID = npc1.GetBodyAction(NULL, "DamageH");
	npc1_DieID = npc1.GetBodyAction(NULL, "HeavyDamage");

	npc2_IdleID = npc2.GetBodyAction(NULL, "CombatIdle");
	npc2_RunID = npc2.GetBodyAction(NULL, "Run");
	npc2_NormalAttack1ID = npc2.GetBodyAction(NULL, "NormalAttack1");
	npc2_NormalAttack2ID = npc2.GetBodyAction(NULL, "NormalAttack2");
	npc2_HeavyAttack1ID = npc2.GetBodyAction(NULL, "HeavyAttack1");
	npc2_Damage1ID = npc2.GetBodyAction(NULL, "Damage1");
	npc2_Damage2ID = npc2.GetBodyAction(NULL, "Damage2");
	npc2_DieID = npc2.GetBodyAction(NULL, "Die");

	// set the character to idle action
	CurPoseID = IdleID;
	actor.SetCurrentAction(NULL, 0, CurPoseID);
	actor.Play(START, 0.0f, FALSE, TRUE);
	actor.TurnRight(90.0f);

	npc1.SetCurrentAction(NULL, 0, npc1_IdleID);
	npc1.Play(START, 0.0f, FALSE, TRUE);
	npc1.TurnRight(90.0f);

	npc2_CurPoseID = npc2_IdleID;
	npc2.SetCurrentAction(NULL, 0, npc2_CurPoseID);
	npc2.Play(START, 0.0f, FALSE, TRUE);
	npc2.TurnRight(90.0f);

	// translate the camera
	cID = scene.CreateObject(CAMERA);
	FnCamera camera;
	camera.ID(cID);
	camera.SetNearPlane(5.0f);
	camera.SetFarPlane(100000.0f);

	// set camera initial position and orientation
	pos[0] = 4315.783f; pos[1] = -3199.686f; pos[2] = 93.046f;
	fDir[0] = -0.983f; fDir[1] = -0.143f; fDir[2] = -0.119f;
	uDir[0] = -0.116f; uDir[1] = -0.031f; uDir[2] = 0.993f;
	camera.SetPosition(pos);
	camera.SetDirection(fDir, uDir);
	Camera3PersonView(TRUE); // default face back

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
	FyDefineHotKey(FY_DOWN, Movement, FALSE);	 // Back for moving back
	FyDefineHotKey(FY_Z, ActorAttack, FALSE);	 // Normal Attack
	FyDefineHotKey(FY_X, ActorAttack, FALSE);	 // Heavy Attack
	FyDefineHotKey(FY_C, ActorAttack, FALSE);	// Ultimate Attack

	// define some mouse functions
	FyBindMouseFunction(LEFT_MOUSE, InitPivot, PivotCam, NULL, NULL);
	FyBindMouseFunction(MIDDLE_MOUSE, InitZoom, ZoomCam, NULL, NULL);
	FyBindMouseFunction(RIGHT_MOUSE, InitMove, MoveCam, NULL, NULL);

	// bind timers, frame rate = 30 fps
	FyBindTimer(0, 30.0f, GameAI, TRUE);
	FyBindTimer(1, 30.0f, RenderIt, TRUE);

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
	FnCharacter npc1;
	FnCharacter npc2;
	float speed = 10.0f;
	float rotate = 5.0f;

	// play character pose
	actor.ID(actorID);
	npc1.ID(npc1ID);
	npc2.ID(npc2ID);
	CurPoseID = actor.GetCurrentAction(NULL);
	if (CurPoseID == IdleID)
	{
		actor.Play(LOOP, (float)skip, FALSE, TRUE);
	}
	else if (CurPoseID == RunID)
	{
		actor.Play(LOOP, (float)skip, FALSE, TRUE);
	}
	else if (CurPoseID == NormalAttack1ID || 
			 CurPoseID == NormalAttack2ID || 
			 CurPoseID == NormalAttack3ID ||
			 CurPoseID == NormalAttack4ID ||
			 CurPoseID == HeavyAttack1ID  ||
			 CurPoseID == HeavyAttack2ID  ||
			 CurPoseID == HeavyAttack3ID  ||
			 CurPoseID == UltimateAttackID)
	{
		isNPCHit();
		if (!actor.Play(ONCE, (float)skip, TRUE)){
			if (isCombo)
			{
				actor.SetCurrentAction(NULL, 0, NextAttackID);
				isCombo = false;
			}
			else{
				actor.SetCurrentAction(NULL, 0, IdleID);
			}
		}
	}
	CurPoseID = npc1.GetCurrentAction(NULL, 0);
	if (CurPoseID == npc1_IdleID)
	{
		npc1.Play(LOOP, (float)skip, FALSE, TRUE);
	}
	else if (CurPoseID == npc1_Damage1ID)
	{
		if (!npc1.Play(ONCE, (float)skip, FALSE, TRUE))
		{
			npc1.SetCurrentAction(NULL, 0, npc1_IdleID);
		}
	}
	npc2.Play(LOOP, (float)skip, FALSE, TRUE);

	// camera
	float apos[3], cpos[3]; //actor,camera position
	float fDir[3], uDir[3]; //actor face, up, right dir;
	float rDir[3];
	float cfDir[3], cuDir[3]; // camera face, up dir
	float distance = 500, height = 100;
	FnCamera camera;
	camera.ID(cID);
	camera.GetPosition(cpos);
	camera.GetDirection(cfDir, cuDir);

	// front
	if (FyCheckHotKeyStatus(FY_UP)) {
		// get new actor face dir
		fDir[0] = cfDir[0];
		fDir[1] = cfDir[1];
		fDir[2] = 0;
		uDir[0] = 0;
		uDir[1] = 0;
		uDir[2] = 1;
		actor.SetDirection(fDir, uDir);
		actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
		// get actor position & direction
		actor.GetPosition(apos);
		actor.GetDirection(fDir, uDir);
		// calculate camera position & direction
		// camera pos
		cpos[0] = apos[0] - fDir[0] * distance;
		cpos[1] = apos[1] - fDir[1] * distance;
		cpos[2] = apos[2] + uDir[2] * height;
		// camera set
		camera.SetPosition(cpos);
		// collision cam modification
		CameraCollision();
	}
	// left circle
	else if (FyCheckHotKeyStatus(FY_LEFT)) {
		// set actor face left then move forward
		cross3(fDir, cuDir, cfDir);
		uDir[0] = 0;
		uDir[1] = 0;
		uDir[2] = 1;
		actor.SetDirection(fDir, uDir);
		actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
		// camera rotate
		// get actor position & direction
		actor.GetPosition(apos);
		actor.GetDirection(fDir, uDir);
		// calculate camera position & direction
		// camera face
		cfDir[0] = apos[0] - cpos[0];
		cfDir[1] = apos[1] - cpos[1];
		cfDir[2] = apos[2] - cpos[2] + height / 2;
		// camera up
		cross3(cuDir, cfDir, fDir);
		// camera set
		camera.SetDirection(cfDir, cuDir);
	}
	// right circle
	else if (FyCheckHotKeyStatus(FY_RIGHT)) {
		// set actor face right then move forward
		cross3(fDir, cfDir, cuDir);
		uDir[0] = 0;
		uDir[1] = 0;
		uDir[2] = 1;
		actor.SetDirection(fDir, uDir);
		actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
		// camera rotate
		// get actor position & direction
		actor.GetPosition(apos);
		actor.GetDirection(fDir, uDir);
		// calculate camera position & direction
		// camera face
		cfDir[0] = apos[0] - cpos[0];
		cfDir[1] = apos[1] - cpos[1];
		cfDir[2] = apos[2] - cpos[2] + height / 2;
		// camera up
		cross3(cuDir, fDir, cfDir);
		// camera set
		camera.SetDirection(cfDir, cuDir);
	}
	// back
	else if (FyCheckHotKeyStatus(FY_DOWN)) {
		// get new actor face dir
		fDir[0] = -cfDir[0];
		fDir[1] = -cfDir[1];
		fDir[2] = 0;
		uDir[0] = 0;
		uDir[1] = 0;
		uDir[2] = 1;
		actor.SetDirection(fDir, uDir);
		actor.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE);
		// get actor position & direction
		actor.GetPosition(apos);
		actor.GetDirection(fDir, uDir);
		// calculate camera position & direction
		// camera pos
		cpos[0] = apos[0] + fDir[0] * distance;
		cpos[1] = apos[1] + fDir[1] * distance;
		cpos[2] = apos[2] + uDir[2] * height;
		// camera set
		camera.SetPosition(cpos);
		// collision cam modification
		CameraCollision();
	}

	if (actorAttacking == 1){
		actorAttackFrame++;
		/*if (actorAttackFrame > 30){
			actorAttacking = 0;
			actor.ID(actorID);
			CurPoseID = IdleID;
			actor.SetCurrentAction(NULL, 0, CurPoseID);
			actor.Play(START, 0.0f, FALSE, TRUE);
		}*/
	}

}

//
void Camera3PersonView(int skip)
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
	cpos[0] = apos[0] - fDir[0] * distance;
	cpos[1] = apos[1] - fDir[1] * distance;
	cpos[2] = apos[2] + uDir[2] * height;
	// camera face
	cfDir[0] = apos[0] - cpos[0];
	cfDir[1] = apos[1] - cpos[1];
	cfDir[2] = apos[2] - cpos[2] + height / 2;
	// camera up
	cross3(rDir, fDir, uDir);
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
	cross3(rDir, fDir, uDir);
	cross3(cuDir, cfDir, rDir);
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

	//get npc1's data
	FnCharacter npc1;
	FnObject objNPC1;
	npc1.ID(npc1ID);
	objNPC1.ID(npc1.GetBaseObject());

	float npc1Pos[3],npc1Pos2[3];
	npc1.GetPosition(npc1Pos);
	objNPC1.GetPosition(npc1Pos2);
	float ray[3];
	ray[0] = actorPos[0] - weaponPos[0];
	ray[1] = actorPos[1] - weaponPos[1];
	float result = abs(ray[1] * npc1Pos[0] - ray[0] * npc1Pos[1] + weaponPos[1] * actorPos[0] - weaponPos[0] * actorPos[1]) /
		sqrt(ray[0] * ray[0] + ray[1] * ray[1]);

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

	char actorPosS[256], weaponPosS[256],npc1PosS[256],resultsS[256];
	sprintf(actorPosS, "actor pos: %8.3f %8.3f %8.3f", actorPos[0], actorPos[1], actorPos[2]);
	sprintf(weaponPosS, "weapon pos: %8.3f %8.3f %8.3f", weaponPos[0], weaponPos[1], weaponPos[2]);
	sprintf(npc1PosS, "npc1 pos: %8.3f %8.3f %8.3f", npc1Pos[0], npc1Pos[1], npc1Pos[2]);
	sprintf(resultsS, "results: %8.3f %8.3f %8.3f %8.3f", ray[0], ray[1], dist2(actorPos,npc1Pos),result);

	text.Write(actorPosS, 20, 80, 255, 255, 0);
	text.Write(weaponPosS, 20, 100, 255, 255, 0);
	text.Write(npc1PosS, 20, 120, 255, 255, 0);
	text.Write(resultsS, 20, 140, 255, 255, 0);


	text.End();

	// swap buffer
	FySwapBuffers();
}

/*------------------
movement control
-------------------*/
int stack = 0; // keep track of multi key press
void Movement(BYTE code, BOOL4 value)
{
	//===============================
	//		Homework #01 part 2
	//===============================
	FnCharacter actor;
	actor.ID(actorID);
	if (!value) {
		stack--;
		if (stack <= 0) {
			CurPoseID = IdleID;
			actor.SetCurrentAction(NULL, 0, CurPoseID);
			actor.Play(START, 0.0f, FALSE, TRUE);
		}
	}
	else {
		// (code == FY_UP || code == FY_LEFT || code == FY_RIGHT) 
		stack++;
		CurPoseID = RunID;
		actor.SetCurrentAction(NULL, 0, CurPoseID);
		actor.Play(START, 0.0f, FALSE, TRUE);
	}
}

void ActorAttack(BYTE code, BOOL4 value)
{
	if (!value) return;
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

void isNPCHit()
{
	FnCharacter actor;
	FnCharacter npc1;
	FnCharacter npc2;
	FnObject ObjNPC1;
	FnObject ObjNPC2;
	FnObject actorWeapon;
	float weaponPos[3],actorPos[3],npc1Pos[3];
	float ray[3];

	actor.ID(actorID);
	npc1.ID(npc1ID);
	npc2.ID(npc2ID);
	ObjNPC1.ID(npc1.GetBaseObject());
	ObjNPC2.ID(npc2.GetBaseObject());
	actorWeapon.ID(actor.GetBoneObject("WeaponDummy03"));
	actorWeapon.GetPosition(weaponPos);
	npc1.GetPosition(npc1Pos);
	actor.GetPosition(actorPos);
	ray[0] = actorPos[0] - weaponPos[0];
	ray[1] = actorPos[1] - weaponPos[1];
	ray[2] = 0;

	CurPoseID = npc1.GetCurrentAction(NULL, 0);

	// distance from npc to actor's attack range
	float d1 = abs(ray[1] * npc1Pos[0] - ray[0] * npc1Pos[1] + weaponPos[1] * actorPos[0] - weaponPos[0] * actorPos[1]) /
			  sqrt(ray[0] * ray[0] + ray[1] * ray[1]);

	// disance from npc to actor
	float d2 = dist2(npc1Pos, actorPos);
	
	// distance from npc to actor's weapon
	float d3 = dist2(npc1Pos, weaponPos);
	// distance from actor to its' weapon
	float d4 = dist2(actorPos, weaponPos);

	if (  d1 < 20					  &&
		  sqrt(d2*d2-d1*d1) < d4	  &&
		  sqrt(d3*d3-d1*d1) < d4	  )
	{
		npc1.SetCurrentAction(NULL, 0, npc1_Damage1ID);
		npc1.Play(START, 0.0f,FALSE,TRUE);
	}

	if (ObjNPC2.HitTest(weaponPos, ray) > 0)
	{
		npc2.SetCurrentAction(NULL, 0, npc2_Damage2ID);
		npc1.Play(START, 0.0f, FALSE, TRUE);
	}

}