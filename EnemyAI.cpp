#include "EnemyAI.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common/NavigationGrid.h"

using namespace NCL;
using namespace CSC8503;

// returns true if path from src to dst can be found and assigns direction to move in
bool EnemyAI::Pathfind(Vector3& pathDir, Vector3& src, Vector3& dst) {
	NavigationGrid grid(mapGrid);

	NavigationPath outPath;

	Vector3 offset = Vector3(205.0f, 0.0f, 495.0f);

	Vector3 startPos = src + offset;
	Vector3 endPos = dst + offset;

	bool found = false;
	found = grid.FindPath(startPos, endPos, outPath);

	// if path found, assign direction to next point
	if (found) {
		updatePath = 0.0f;
		outPath.PopWaypoint(currentPos);
		outPath.PopWaypoint(nextPos);

		pathDirection = nextPos - currentPos;
		pathDirection.Normalise();
		pathDir = pathDirection;
	}
	return found;
}

// setup chase AI state machine with states, state transitions and behaviour
void EnemyAIChase::SetupStateMachine() {
	EnemyFunc idleFunc = [](void* enemy, void* player) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		enemyObject->SetStateDescription("idle");
	};

	// @TODO raycast for this?
	EnemyFunc spottedPlayer = [](void* enemy, void* player) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		GameObject* playerObject = (GameObject*)player;
		enemyObject->SetStateDescription("spotted player");
		enemyObject->GetTransform().SetLocalOrientation(enemyObject->GetOrientationToPosition(playerObject->GetTransform().GetWorldPosition()));
	};

	EnemyFunc chaseFunc = [](void* enemy, void* player) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		GameObject* playerObject = (GameObject*)player;
		enemyObject->SetStateDescription("chasing");

		Vector3 enemyPos = enemyObject->GetTransform().GetWorldPosition();
		Vector3 playerPos = playerObject->GetTransform().GetWorldPosition();
		Vector3 dir = playerPos - enemyPos;
		dir.Normalise();
		enemyObject->GetTransform().SetLocalOrientation(enemyObject->GetOrientationToPosition(playerPos));

		// pathfind to player, if pathfind fails just use direction vector between player and AI
		Vector3 pathDir;
		if(enemyObject->Pathfind(pathDir, enemyPos, playerPos))
			enemyObject->GetPhysicsObject()->AddForce((pathDir * enemyObject->GetSpeedMultiplier()) * Vector3(1.0, 0.0, 1.0));
		else
			enemyObject->GetPhysicsObject()->AddForce((dir * enemyObject->GetSpeedMultiplier()) * Vector3(1.0, 0.0, 1.0));
	};

	// @TODO placeholder until decided on how AI should behave on collision with player/ball
	EnemyFunc stop = [](void* enemy, void* data) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		enemyObject->SetStateDescription("stopped");
		enemyObject->GetPhysicsObject()->ClearForces();
	};

	// setup states
	EnemyState* idleState = new EnemyState(idleFunc, this);
	EnemyState* spottedState = new EnemyState(spottedPlayer, this, chaseObject);
	EnemyState* chaseState = new EnemyState(chaseFunc, this, chaseObject);
	EnemyState* stoppedState = new EnemyState(stop, this);

	sM->AddState(idleState);
	sM->AddState(spottedState);
	sM->AddState(chaseState);
	sM->AddState(stoppedState);

	// setup state transitions

	// idle -> spotted
	GenericTransition<float&, float>* idleToSpotted = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToObject, 50.0f, idleState, spottedState);

	// spotted -> idle
	GenericTransition<float&, float>* spottedToIdle = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, distToObject, 60.0f, spottedState, idleState);

	// spotted -> chase
	GenericTransition<float&, float>* spottedToChase = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToObject, 35.0f, spottedState, chaseState);

	// chase -> spotted
	GenericTransition<float&, float>* chaseToSpotted = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, distToObject, 45.0f, chaseState, spottedState);

	/***********@TODO Placeholder - AI simply stops once close to player/ball, team need to decide on most appropriate response*********************/
	// chase -> stopped
	GenericTransition<float&, float>* chaseToStopped = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToObject, 5.0f, chaseState, stoppedState);

	// stopped -> chase
	GenericTransition<float&, float>* stoppedToChase = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, distToObject, 15.0f, stoppedState, chaseState);
	/***********************************************************************************************************************************************/

	// add to StateMachine
	sM->AddTransition(idleToSpotted);
	sM->AddTransition(spottedToIdle);
	sM->AddTransition(spottedToChase);
	sM->AddTransition(chaseToSpotted);
	sM->AddTransition(chaseToStopped);
	sM->AddTransition(stoppedToChase);
}


// setup patrol AI state machine with states, state transitions and behaviour
void EnemyAIPatrol::SetupStateMachine() {

	EnemyFunc waitFunc = [](void* enemy, void* data) {
		EnemyAIPatrol* enemyObject = (EnemyAIPatrol*)enemy;
		enemyObject->SetStateDescription("waiting");
		if (!enemyObject->GetIsWaiting()) {
			enemyObject->SetIsWaiting(true);
			enemyObject->NextDestination();
		}
	};

	EnemyFunc patrolToPoint = [](void* enemy, void* data) {
		EnemyAIPatrol* enemyObject = (EnemyAIPatrol*)enemy;
		if (enemyObject->GetIsWaiting())
			enemyObject->SetIsWaiting(false);
		Vector3 dest = enemyObject->GetCurrentDestination();
		Vector3 enemyPos = enemyObject->GetTransform().GetWorldPosition();
		enemyObject->SetStateDescription("patrolling");
		enemyObject->GetTransform().SetLocalOrientation(enemyObject->GetOrientationToPosition(dest));

		// pathfind to destination. if pathfind fails, stop
		Vector3 pathDir;
		if (enemyObject->Pathfind(pathDir, enemyPos, dest))
			enemyObject->GetPhysicsObject()->AddForce((pathDir * enemyObject->GetSpeedMultiplier()) * Vector3(1.0, 0.0, 1.0));
		else
			enemyObject->GetPhysicsObject()->ClearForces();
	};

	EnemyFunc chaseFunc = [](void* enemy, void* player) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		GameObject* playerObject = (GameObject*)player;
		enemyObject->SetStateDescription("chasing");
		Vector3 enemyPos = enemyObject->GetTransform().GetWorldPosition();
		Vector3 playerPos = playerObject->GetTransform().GetWorldPosition();
		enemyObject->GetTransform().SetLocalOrientation(enemyObject->GetOrientationToPosition(playerObject->GetTransform().GetWorldPosition()));
		
		// pathfind to player. if pathfind fails, stop
		Vector3 pathDir;
		if (enemyObject->Pathfind(pathDir, enemyPos, playerPos))
			enemyObject->GetPhysicsObject()->AddForce((pathDir * enemyObject->GetSpeedMultiplier()) * Vector3(1.0, 0.0, 1.0));
		else
			enemyObject->GetPhysicsObject()->ClearForces();
	};

	EnemyFunc stop = [](void* enemy, void* data) {
		EnemyAIChase* enemyObject = (EnemyAIChase*)enemy;
		enemyObject->SetStateDescription("stopped");
		enemyObject->GetPhysicsObject()->ClearForces();
	};

	// setup states
	EnemyState* waitState = new EnemyState(waitFunc, this);
	EnemyState* patrolToPointState = new EnemyState(patrolToPoint, this);
	EnemyState* chaseState = new EnemyState(chaseFunc, this, chaseObject);
	EnemyState* stoppedState = new EnemyState(stop, this);

	sM->AddState(waitState);
	sM->AddState(patrolToPointState);
	sM->AddState(chaseState);
	sM->AddState(stoppedState);

	// setup transitions

	// patrol -> wait
	GenericTransition<float&, float>* patrolToWait = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToCurrentDest, 6.0, patrolToPointState, waitState);

	// wait -> patrol
	GenericTransition<float&, float>* waitToPatrol = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, waitTime, 1.0, waitState, patrolToPointState);

	// patrol -> chase
	GenericTransition<float&, float>* patrolToChase = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToObject, 35.0f, patrolToPointState, chaseState);

	// chase -> patrol
	GenericTransition<float&, float>* chaseToPatrol = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, distToObject, 45.0f, chaseState, patrolToPointState);


	/***********@TODO Placeholder - AI simply stops once close to player/ball, team need to decide on most appropriate response*********************/
	// chase -> stopped
	GenericTransition<float&, float>* chaseToStopped = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition, distToObject, 5.0f, chaseState, stoppedState);

	// stopped -> chase
	GenericTransition<float&, float>* stoppedToChase = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition, distToObject, 15.0f, stoppedState, chaseState);
	/***********************************************************************************************************************************************/


	// add to StateMachine
	sM->AddTransition(waitToPatrol);
	sM->AddTransition(patrolToWait);
	sM->AddTransition(patrolToChase);
	sM->AddTransition(chaseToPatrol);
	sM->AddTransition(chaseToStopped);
	sM->AddTransition(stoppedToChase);
}