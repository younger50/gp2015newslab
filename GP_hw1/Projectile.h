#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_
#include"FlyWin32.h"
#include<math.h>

class Projectile{

public:
	Projectile(float *fDir, float r, float sp, float *st, SCENEid sid, char* attack_special){
		FnObject obj;
		FnGameFXSystem fx;
		FnScene scene;
		FnCharacter c;
		scene.ID(sid);
		oid = scene.CreateObject(OBJECT);
		fxid = scene.CreateGameFXSystem();
		fx.ID(fxid);
		fx.Load(attack_special, TRUE);
		fx.SetParentObjectForAll(oid);

		obj.ID(oid);
		obj.SetPosition(st);
		float uDir[3];

		uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
		obj.SetDirection(fDir, uDir);
		obj.PutOnTerrain(st);

		range = r;
		speed = sp;
		start[0] = st[0]; start[1] = st[1]; start[2] = st[2];
		hit = false;
		land = false;

	};
	~Projectile(){};

public:
	/*
	Given a bbox, check if this projectile hit it. 
	*/
	bool hitTest(float* min, float* max);

	/*
	Update position of this projectile.
	*/
	void fly(int skip);

	/*
	Return if this projectile reaches its end.
	*/
	bool island(){ return land; }

	/*
	Return if this proejectile hit anything.
	*/
	bool isHit(){ return hit; }
	/*
	
	*/
	float* getStart(){
		return start;
	}

private:

	OBJECTid oid;
	GAMEFX_SYSTEMid fxid;
	float speed;
	float range;
	float start[3];

	bool hit;
	bool land;

	float dist3(float *a, float *b);

};


#endif