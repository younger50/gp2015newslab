#include"Projectile.h"

bool Projectile::hitTest(float *min, float *max){
	FnObject obj;
	float pos[3];

	obj.ID(oid);
	obj.GetPosition(pos);

	return min[0] < pos[0] && pos[0] < max[0]
		&& min[1] < pos[1] && pos[1] < max[1]
		&& min[2] < pos[2] && pos[2] < max[2];
}

void Projectile::fly(int skip){
	FnObject obj;
	FnGameFXSystem fx(fxid);
	float pos[3];
	fx.Play((float)skip, LOOP);
	obj.ID(oid);
	obj.GetPosition(pos);
	if (dist3(pos, start) > range);
	{
		land = true;
	}

	if (WALK != obj.MoveForward(speed, TRUE, FALSE, 0.0f, TRUE)){
		//hit the wall
		hit = true;
	}

	
}

// 3D point distance
float Projectile::dist3(float *a, float *b){
	return sqrt((a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]) + (a[2] - b[2])*(a[2] - b[2]));
}