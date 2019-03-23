#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"
#include "../client/ClientEffect.h"

#ifndef __GAME_PROJECTILE_H__
#include "../Projectile.h"
#endif


const int NAPALM_GUN_NUM_CYLINDERS =  5;

class WeaponNapalmGun : public rvWeapon {
public:

	CLASS_PROTOTYPE( WeaponNapalmGun );

	WeaponNapalmGun ( void );
	~WeaponNapalmGun ( void );

	virtual void			Spawn				( void );
	virtual void			Think				( void );
	virtual void			MuzzleRise			( idVec3 &origin, idMat3 &axis );
	virtual void			SpectatorCycle		( void );

	void					Save( idSaveGame *saveFile ) const;
	void					Restore( idRestoreGame *saveFile );

protected:

	void					UpdateCylinders(void);
	void					Attack(void);
	//Experience
	int					exp = 0;
	int					total_exp = 1500;
	int					level = 1;
	int					num_attacks = 1;
	float				pow = 1.0f;

	typedef enum {CYLINDER_RESET_POSITION,CYLINDER_MOVE_POSITION, CYLINDER_UPDATE_POSITION } CylinderState;
	CylinderState								cylinderState;

private:

	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_Fire				( const stateParms_t& parms );
	stateResult_t		State_Reload			( const stateParms_t& parms );
	stateResult_t		State_EmptyReload		( const stateParms_t& parms );
	
	stateResult_t		Frame_MoveCylinder		( const stateParms_t& parms );
	stateResult_t		Frame_ResetCylinder		( const stateParms_t& parms );


	float								cylinderMaxOffsets[NAPALM_GUN_NUM_CYLINDERS];
	idInterpolate<float>				cylinderOffsets[NAPALM_GUN_NUM_CYLINDERS];
	jointHandle_t						cylinderJoints[NAPALM_GUN_NUM_CYLINDERS];


	int									cylinderMoveTime;
	int									previousAmmo;
	bool								zoomed;
	
	CLASS_STATES_PROTOTYPE ( WeaponNapalmGun );
};

CLASS_DECLARATION( rvWeapon, WeaponNapalmGun )
END_CLASS

/*
================
WeaponNapalmGun::WeaponNapalmGun
================
*/
WeaponNapalmGun::WeaponNapalmGun( void ) { }

/*
================
WeaponNapalmGun::~WeaponNapalmGun
================
*/
WeaponNapalmGun::~WeaponNapalmGun( void ) { }

/*
================
WeaponNapalmGun::Spawn
================
*/
void WeaponNapalmGun::Spawn( void ) {
	assert(viewModel);
	idAnimator* animator = viewModel->GetAnimator();
	assert(animator);

	SetState( "Raise", 0 );	

    for(int i = 0; i < NAPALM_GUN_NUM_CYLINDERS; ++i)
	{
		idStr argName = "cylinder_offset";
		argName += i;
        cylinderMaxOffsets[i] = spawnArgs.GetFloat(argName, "0.0");

		argName = "cylinder_joint";
		argName += i;
		cylinderJoints[i] = animator->GetJointHandle( spawnArgs.GetString( argName, "" ) );

		cylinderOffsets[i].Init( gameLocal.time, 0.0f, 0, 0);
	}

	previousAmmo = AmmoInClip();
	cylinderMoveTime  = spawnArgs.GetFloat( "cylinderMoveTime", "500" );
	cylinderState = CYLINDER_RESET_POSITION;
	zoomed = false;
}

/*
================
WeaponNapalmGun::Think
================
*/
void WeaponNapalmGun::Think( void ) {

	rvWeapon::Think();

	//Check to see if the ammo level has changed.
	//This is to account for ammo pickups.
	if ( previousAmmo != AmmoInClip() ) {
		// don't do this in MP, the weap script doesn't sync the canisters anyway
		if ( !gameLocal.isMultiplayer ) {
			//change the cylinder state to reflect the new change in ammo.
			cylinderState = CYLINDER_MOVE_POSITION;
		}
		previousAmmo = AmmoInClip();
	}

	UpdateCylinders();
}

/*
===============
WeaponNapalmGun::MuzzleRise
===============
*/
void WeaponNapalmGun::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
	if ( wsfl.zoom )
		return;

	rvWeapon::MuzzleRise( origin, axis );
}

/*
===============
WeaponNapalmGun::UpdateCylinders
===============
*/
void WeaponNapalmGun::UpdateCylinders(void)
{
	idAnimator* animator;
	animator = viewModel->GetAnimator();
	assert( animator );

	float ammoInClip = AmmoInClip();
	float clipSize = ClipSize();
	if ( clipSize <= idMath::FLOAT_EPSILON ) {
		clipSize = maxAmmo;
	}

	for(int i = 0; i < NAPALM_GUN_NUM_CYLINDERS; ++i)
	{
		// move the local position of the joint along the x-axis.
		float currentOffset = cylinderOffsets[i].GetCurrentValue(gameLocal.time);

		switch(cylinderState)
		{
			case CYLINDER_MOVE_POSITION:
				{
					float cylinderMaxOffset = cylinderMaxOffsets[i];
					float endValue = cylinderMaxOffset * (1.0f - (ammoInClip / clipSize));
					cylinderOffsets[i].Init( gameLocal.time, cylinderMoveTime, currentOffset, endValue );
				}
			break;

			case CYLINDER_RESET_POSITION:
				{
					float cylinderMaxOffset = cylinderMaxOffsets[i];
					float endValue = cylinderMaxOffset * (1.0f - (ammoInClip / clipSize));
					cylinderOffsets[i].Init( gameLocal.time, 0, endValue, endValue );
				}
			break;
		}


		animator->SetJointPos( cylinderJoints[i], JOINTMOD_LOCAL, idVec3( currentOffset, 0.0f, 0.0f ) );
	}

	cylinderState = CYLINDER_UPDATE_POSITION;
}


/*
=====================
WeaponNapalmGun::Save
=====================
*/
void WeaponNapalmGun::Save( idSaveGame *saveFile ) const 
{
	for(int i = 0; i < NAPALM_GUN_NUM_CYLINDERS; i++)
	{
		saveFile->WriteFloat(cylinderMaxOffsets[i]);
		saveFile->WriteInterpolate(cylinderOffsets[i]);
		saveFile->WriteJoint(cylinderJoints[i]);
	}

	saveFile->WriteInt(cylinderMoveTime);
	saveFile->WriteInt(previousAmmo);
}

/*
=====================
WeaponNapalmGun::Restore
=====================
*/
void WeaponNapalmGun::Restore( idRestoreGame *saveFile ) {
	
	for(int i = 0; i < NAPALM_GUN_NUM_CYLINDERS; i++)
	{
		saveFile->ReadFloat(cylinderMaxOffsets[i]);
		saveFile->ReadInterpolate(cylinderOffsets[i]);
		saveFile->ReadJoint(cylinderJoints[i]);
	}

	saveFile->ReadInt(cylinderMoveTime);
	saveFile->ReadInt(previousAmmo);
}

/*
=====================
WeaponNapalmGun::Attack 
added by js986
=====================
*/
void WeaponNapalmGun::Attack(void) {
	const idDeclEntityDef* napalm = gameLocal.FindEntityDef("projectile_napalm", false);
	idEntity* n;
	idPlayer* player = gameLocal.GetLocalPlayer();
	if (!gameLocal.isClient) {
		// check if we're out of ammo or the clip is empty
		int ammoAvail = owner->inventory.HasAmmo(ammoType, ammoRequired);
		if (!ammoAvail || ((clipSize != 0) && (ammoClip <= 0))) {
			return;
		}
		owner->inventory.UseAmmo(ammoType, ammoRequired);
		if (clipSize && ammoRequired) {
			clipPredictTime = gameLocal.time;	// mp client: we predict this. mark time so we're not confused by snapshots
			ammoClip -= 1;
		}

		// wake up nearby monsters
		if (!wfl.silent_fire) {
			gameLocal.AlertAI(owner);
		}
	}
	if (napalm && (owner == player)){
			gameLocal.SpawnEntityDef(napalm->dict, &n);
			idProjectile* fireball = static_cast<idProjectile*>(n);
			float num = 1.0f;
			idVec3 start;
			idMat3 axis;
			idVec3 dir;
			idVec3 dirOffset;
			idVec3 startOffset;
			//idVec3 x(num, 0.0f, 0.0f);
			if (barrelJointView != INVALID_JOINT && spawnArgs.GetBool("launchFromBarrel")) {
				GetGlobalJointTransform(true, barrelJointView, start, axis);
			}
			else {
				start = playerViewOrigin;
				axis = playerViewAxis;
				start += playerViewAxis[0] * muzzleOffset;
			}
			float spreadRad = (num*DEG2RAD(spread));
			float ang = idMath::Sin(spreadRad * gameLocal.random.RandomFloat());
			float spin = (float)DEG2RAD(360.0f) * gameLocal.random.RandomFloat();
			spawnArgs.GetVector("dirOffset", "0 0 0", dirOffset);
			spawnArgs.GetVector("startOffset", "0 0 0", startOffset);
			dir = playerViewAxis[0] + playerViewAxis[2] * (ang * idMath::Sin(spin)) - playerViewAxis[1] * (ang * idMath::Cos(spin));
			dir += dirOffset;
			// Inform the gui of the ammo change
			viewModel->PostGUIEvent("weapon_ammo");
			if (ammoClip == 0 && AmmoAvailable() == 0) {
				viewModel->PostGUIEvent("weapon_noammo");
			}
			fireball->Create(player, start + startOffset, dir, NULL);
			fireball->Launch(start, dir, player->weapon->pushVelocity, 0.0f, pow);
			num += 1.0f;
			statManager->WeaponFired(owner, weaponIndex, num_attacks);

		}
		//gameLocal.Printf("Spawn Hype!!");

	

}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( WeaponNapalmGun )
	STATE ( "Idle",				WeaponNapalmGun::State_Idle)
	STATE ( "Fire",				WeaponNapalmGun::State_Fire )
	STATE ( "Reload",			WeaponNapalmGun::State_Reload )
	STATE ( "EmptyReload",		WeaponNapalmGun::State_EmptyReload )
	STATE ( "MoveCylinder",		WeaponNapalmGun::Frame_MoveCylinder )
	STATE ( "ResetCylinder",	WeaponNapalmGun::Frame_ResetCylinder)
END_CLASS_STATES



stateResult_t WeaponNapalmGun::State_Reload( const stateParms_t& parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			PlayAnim ( ANIMCHANNEL_ALL, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGrenadeLauncher::State_Reload
================
*/
stateResult_t WeaponNapalmGun::State_EmptyReload( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( wsfl.netReload ) {
				wsfl.netReload = false;
			} else {
				NetReload ( );
			}
			
			SetStatus ( WP_RELOAD );
			PlayAnim ( ANIMCHANNEL_ALL, "reload_empty", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				AddToClip ( ClipSize() );

				cylinderState = CYLINDER_MOVE_POSITION;

				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
WeaponNapalmGun::State_Idle
================
*/
stateResult_t WeaponNapalmGun::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( AmmoAvailable ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}
		
			if ( wsfl.zoom )
				PlayCycle( ANIMCHANNEL_LEGS, "altidle", parms.blendFrames );
			else
				PlayCycle( ANIMCHANNEL_LEGS, "idle", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}		

			if ( wsfl.zoom && !zoomed ) {
				SetState ( "Idle", 4 );
				zoomed = true;
				return SRESULT_DONE;
			}
			
			if ( !wsfl.zoom && zoomed ) {
				SetState ( "Idle", 4 );
				zoomed = false;
				return SRESULT_DONE;
			}

			if(!clipSize)
			{
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoAvailable ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			}
			else
			{
				if ( wsfl.attack && AutoReload() && !AmmoInClip ( ) && AmmoAvailable () ) {
					SetState ( "EmptyReload", 4 );
					return SRESULT_DONE;			
				}

				if ( wsfl.netReload || (wsfl.reload && AmmoInClip() < ClipSize() && AmmoAvailable()>AmmoInClip()) ) {
					SetState ( "EmptyReload", 4 );
					return SRESULT_DONE;			
				}

				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip ( ) ) {
					SetState ( "Fire", 2 );
					return SRESULT_DONE;
				}
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
WeaponNapalmGun::State_Fire
================
*/
stateResult_t WeaponNapalmGun::State_Fire( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACKLOOP,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( wsfl.zoom ) {
				nextAttackTime = gameLocal.time + (altFireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
				//Attack ( true, 1, spread, 0, 1.0f );

				PlayAnim ( ANIMCHANNEL_ALL, "idle", parms.blendFrames );
				//fireHeld = true;
			} else {
				nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
				//if (!wsfl.attack || wsfl.lowerWeapon)
					//SRESULT_STAGE(STAGE_WAIT);
				//Attack (  );

				
				int animNum = viewModel->GetAnimator()->GetAnim ( "fire" );
				if ( animNum ) {
					idAnim* anim;
					anim = (idAnim*)viewModel->GetAnimator()->GetAnim ( animNum );				
					anim->SetPlaybackRate ( (float)anim->Length() / (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE )) );
				}

				PlayAnim ( ANIMCHANNEL_ALL, "fire", parms.blendFrames );
			}

			previousAmmo = AmmoInClip();
			//return SRESULT_WAIT;
			//return SRESULT_STAGE ( STAGE_ATTACKLOOP );
		case STAGE_ATTACKLOOP:
			if (!wsfl.attack || wsfl.lowerWeapon || !AmmoAvailable()) {
				return SRESULT_STAGE(STAGE_WAIT);
			}
			Attack();
			exp += 0.5; // the following was added by js986
			if (exp >= total_exp) {
				idPlayer* player = gameLocal.GetLocalPlayer();
				level++;
				total_exp = total_exp * 3;
				num_attacks = num_attacks + 2;
				pow += 1.0f;
				if (player && player->hud){
					if (level == 5) {
						player->hud->SetStateString("message", "The Pyrocitor has reached Max Level");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 2) {
						player->hud->SetStateString("message", "The Pyrocitor has reached Level 2");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 3) {
						player->hud->SetStateString("message", "The Pyrocitor has reached Level 3");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 4) {
						player->hud->SetStateString("message", "The Pyrocitor has reached Level 4");
						player->hud->HandleNamedEvent("Message");
					}
				}
			}
			return SRESULT_WAIT;
	
		case STAGE_WAIT:			
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				if ( !wsfl.zoom ) 
					SetState ( "Reload", 4 );
				else
					SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t WeaponNapalmGun::Frame_MoveCylinder( const stateParms_t& parms) {
	cylinderState = CYLINDER_MOVE_POSITION;
	return SRESULT_OK;
}

stateResult_t WeaponNapalmGun::Frame_ResetCylinder( const stateParms_t& parms) {
	cylinderState = CYLINDER_RESET_POSITION;
	return SRESULT_OK;
}

void WeaponNapalmGun::SpectatorCycle( void ) {
	cylinderState = CYLINDER_RESET_POSITION;
}
