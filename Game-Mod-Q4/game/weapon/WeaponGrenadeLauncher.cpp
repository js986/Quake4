#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

#ifndef __GAME_PROJECTILE_H__
#include "../Projectile.h"
#endif

#include "../ai/AI_Tactical.h"
#include "../ai/AI.h"
class rvWeaponGrenadeLauncher : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponGrenadeLauncher );

	rvWeaponGrenadeLauncher ( void );
	~rvWeaponGrenadeLauncher(void);
	virtual void			Spawn				( void );
	void					PreSave				( void );
	void					PostSave			( void );
//Weapon Experience
protected:
	rvClientEntityPtr<rvClientEffect>	guideEffect;
	idList< idEntityPtr<idEntity> >		guideEnts;
	float								guideSpeedSlow;
	float								guideSpeedFast;
	float								guideRange;
	float								guideAccelTime;

	int					exp = 0;
	int					total_exp = 1500;
	int					level = 1;
	int					num_attacks = 1;
	float				pow = 1.0f;

#ifdef _XENON
	virtual bool		AllowAutoAim			( void ) const { return false; }
#endif

private:

	stateResult_t		State_Idle		( const stateParms_t& parms );
	stateResult_t		State_Fire		( const stateParms_t& parms );
	stateResult_t		State_Reload	( const stateParms_t& parms );

	const char*			GetFireAnim() const { return (!AmmoInClip()) ? "fire_empty" : "fire"; }
	const char*			GetIdleAnim() const { return (!AmmoInClip()) ? "idle_empty" : "idle"; }
	
	CLASS_STATES_PROTOTYPE ( rvWeaponGrenadeLauncher );
};

CLASS_DECLARATION( rvWeapon, rvWeaponGrenadeLauncher )
END_CLASS

/*
================
rvWeaponGrenadeLauncher::rvWeaponGrenadeLauncher
================
*/
rvWeaponGrenadeLauncher::rvWeaponGrenadeLauncher ( void ) {
}

/*
================
rvWeaponGrenadeLauncher::rvWeaponGrenadeLauncher
================
*/
rvWeaponGrenadeLauncher::~rvWeaponGrenadeLauncher( void ) {
	if (guideEffect) {
		guideEffect->Stop();
	}
}


/*
================
rvWeaponGrenadeLauncher::Spawn
================
*/
void rvWeaponGrenadeLauncher::Spawn ( void ) {
	SetState ( "Raise", 0 );	
}

/*
================
rvWeaponGrenadeLauncher::PreSave
================
*/
void rvWeaponGrenadeLauncher::PreSave ( void ) {
}

/*
================
rvWeaponGrenadeLauncher::PostSave
================
*/
void rvWeaponGrenadeLauncher::PostSave ( void ) {
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponGrenadeLauncher )
	STATE ( "Idle",		rvWeaponGrenadeLauncher::State_Idle)
	STATE ( "Fire",		rvWeaponGrenadeLauncher::State_Fire )
	STATE ( "Reload",	rvWeaponGrenadeLauncher::State_Reload )
END_CLASS_STATES

/*
================
rvWeaponGrenadeLauncher::State_Idle
================
*/
stateResult_t rvWeaponGrenadeLauncher::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !AmmoAvailable ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}
		
			PlayCycle( ANIMCHANNEL_ALL, GetIdleAnim(), parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:			
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}		
			if ( !clipSize ) {
				if ( wsfl.attack && AmmoAvailable ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			} else { 
				if ( gameLocal.time > nextAttackTime && wsfl.attack && AmmoInClip ( ) ) {
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}  
						
				if ( wsfl.attack && AutoReload() && !AmmoInClip ( ) && AmmoAvailable () ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}
				if ( wsfl.netReload || (wsfl.reload && AmmoInClip() < ClipSize() && AmmoAvailable()>AmmoInClip()) ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGrenadeLauncher::State_Fire
================
*/
stateResult_t rvWeaponGrenadeLauncher::State_Fire ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	//const idDeclEntityDef* mr_zurkon = gameLocal.FindEntityDef("ai_tactical",false);
	const idDeclEntityDef* mr_zurkon = gameLocal.FindEntityDef("projectile_big_grenade", false);
	idEntity* ent;
	idItem* zu;
	switch ( parms.stage ) {
		case STAGE_INIT:
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier ( PMOD_FIRERATE ));
			Attack ( false, num_attacks, spread, 0, pow );

			if (mr_zurkon && (owner == gameLocal.GetLocalPlayer())) {
				gameLocal.SpawnEntityDef(mr_zurkon->dict, &ent, false);
				gameLocal.Printf("SHOULD SPAWN SOMETHING");
				idProjectile* big_grenade = static_cast<idProjectile*>(ent);
				idVec3 muzzleOrigin = playerViewOrigin;
				idVec3 startOffset;
				idVec3 dir;
				idVec3 dirOffset;
				float spreadRad = DEG2RAD(spread);
				for (int i = 0; i < 3; i++){
					float ang = idMath::Sin(spreadRad * gameLocal.random.RandomFloat());
					float spin = (float)DEG2RAD(360.0f) * gameLocal.random.RandomFloat();
					spawnArgs.GetVector("dirOffset", "0 0 0", dirOffset);
					spawnArgs.GetVector("startOffset", "0 0 0", startOffset);
					dir = playerViewAxis[0] + playerViewAxis[2] * (ang * idMath::Sin(spin)) - playerViewAxis[1] * (ang * idMath::Cos(spin));
					dir += dirOffset;
					big_grenade->Create(owner, muzzleOrigin + startOffset, dir, NULL);
					big_grenade->Launch(muzzleOrigin, dir, owner->weapon->pushVelocity, 0.0f, pow);
				}
				trace_t t = trace_t();
				//t.endpos = big_grenade->endpos;
				t.endAxis = big_grenade->GetPhysics()->GetAxis();
				//ent->SetName("Mr.Zurkon");
				//ent->Spawn();
				//zu = static_cast<idItem*>(ent);
				//zu->Spawn();
				const idDeclEntityDef* small_nades = gameLocal.FindEntityDef("projectile_grenade", false);
				if (small_nades && (owner == gameLocal.GetLocalPlayer())) {
					idEntity* grenade;
					float angle = (360.0f / 6.0f);
					idMat3 normalMat = big_grenade->GetPhysics()->GetAxis();
					idVec3 normal = normalMat[0];
					idVec3 axis = normalMat[1];
					normal.Normalize();
					/*
						for (int i = 0; i < 6; i++){
							idMat3 axisMat;
							gameLocal.SpawnEntityDef(small_nades->dict, &grenade);
							idProjectile* sg = static_cast<idProjectile*>(grenade);
							axisMat = axis.ToMat3();
							axisMat.RotateArbitrary(normal, angle * i);

							sg->Create(big_grenade->GetOwner(), big_grenade->GetPhysics()->GetOrigin(), axisMat[0], big_grenade);
							sg->Launch(big_grenade->GetPhysics()->GetOrigin(), axisMat[0], axisMat[0]);

						}
					*/
					
				}
				//big_grenade->GetPhysics()->GetOrigin();
			}

			exp += 100; // the following was added by js986
			if (exp >= total_exp) {
				level++;
				total_exp = total_exp * 2;
				num_attacks = num_attacks + 2;
				pow += 1.0f;
				idPlayer* player = gameLocal.GetLocalPlayer();
				if (player && player->hud){
					if (level == 5) {
						player->hud->SetStateString("message", "The Heavy Bouncer has reached Max Level");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 2) {
						player->hud->SetStateString("message", "The Heavy Bouncer has reached Level 2");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 3) {
						player->hud->SetStateString("message", "The Heavy Bouncer has reached Level 3");
						player->hud->HandleNamedEvent("Message");
					}
					else if (level == 4) {
						player->hud->SetStateString("message", "The Heavy Bouncer has reached Level 4");
						player->hud->HandleNamedEvent("Message");
					}
				}
			}
			PlayAnim ( ANIMCHANNEL_ALL, GetFireAnim(), 0 );	
			return SRESULT_STAGE ( STAGE_WAIT );
	
		case STAGE_WAIT:		
			if ( wsfl.attack && gameLocal.time >= nextAttackTime && AmmoInClip() && !wsfl.lowerWeapon ) {
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetState ( "Idle", 0 );
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
stateResult_t rvWeaponGrenadeLauncher::State_Reload ( const stateParms_t& parms ) {
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
			PlayAnim ( ANIMCHANNEL_ALL, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				AddToClip ( ClipSize() );
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
			
