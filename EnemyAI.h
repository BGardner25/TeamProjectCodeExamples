#pragma once
#include "GameObject.h"
#include "../CSC8503Common/StateMachine.h"
#include <vector>

/*
*	@Author - BaileyGardner
*	EnemyAI class for a variety of AI used for the golf game
*	
*	Handles: behaviour, AI state machine, pathfinding
*/

namespace NCL {
	namespace CSC8503 {
		class NetworkObject;

		// base class for AI
		class EnemyAI : public GameObject {
		public:
			EnemyAI(string name = "", string map = "") : GameObject(name), sM(new StateMachine()), mapGrid(map) {}

			virtual ~EnemyAI() { delete sM; }

			virtual void Update(float dt) {}

			void SetSpeedMultiplier(const float multiplier) { speedMultiplier = multiplier; }
			float GetSpeedMultiplier() const { return speedMultiplier; }

			void SetStateDescription(const string description) { stateDescription = description; }
			string GetStateDescription() const { return stateDescription; }

			bool Pathfind(Vector3& pathDir, Vector3& src, Vector3& dst);

			// returns orientation between this EnemyAI and pos
			Quaternion GetOrientationToPosition(Vector3 pos) {
				Vector3 dir = pos - this->GetTransform().GetWorldPosition();
				dir.Normalise();
				float dirAngle = atan2(dir.x, dir.z);
				return Quaternion(0.0f, sin(dirAngle * 0.5f), 0.0f, cos(dirAngle * 0.5f));
			}
		protected:
			StateMachine* sM;
			string stateDescription = "";
			string mapGrid;
			float speedMultiplier = 100.0f;

			// how often to check for new path
			float updatePath = 0.51;
			Vector3 currentPos;
			Vector3 nextPos;
			Vector3 pathDirection;
			
			virtual void SetupStateMachine() {}
		};


		// AI remains still until player is within chasing distance, then chases. stops when player moves out of range
		class EnemyAIChase : public EnemyAI {
		public:
			EnemyAIChase(GameObject* chaseObject, string name = "", string map = "") : EnemyAI(name, map), chaseObject(chaseObject), distToObject(0) {
				SetupStateMachine();
			}

			~EnemyAIChase() {}

			void Update(float dt) {
				updatePath += dt;
				distToObject = (chaseObject->GetTransform().GetWorldPosition() - GetTransform().GetWorldPosition()).Length();
				sM->Update();
			}
		private:
			GameObject* chaseObject;
			float distToObject;

			void SetupStateMachine();
		};


		// patrols between a set of given points. if player gets close, chases player. if player moves out of range, returns back to patrolling.
		class EnemyAIPatrol : public EnemyAI {
		public:
			EnemyAIPatrol(std::vector<Vector3> points, GameObject* chaseObject = nullptr, string name = "", string map = "") : EnemyAI(name, map), chaseObject(chaseObject), points(points) {
				isWaiting = true;
				hasReachedEnd = false;
				waitTime = 0.0f;
				index = 0;
				currentDest = points[index];
				distToCurrentDest = (currentDest - GetTransform().GetWorldPosition()).Length();
				distToObject = 30.0f;
				SetupStateMachine();
			}

			~EnemyAIPatrol() {}

			void Update(float dt) {
				updatePath += dt;
				if (isWaiting)
					waitTime += dt;
				distToCurrentDest = (currentDest - GetTransform().GetWorldPosition()).Length();
				distToObject = (chaseObject->GetTransform().GetWorldPosition() - GetTransform().GetWorldPosition()).Length();
				sM->Update();
			}

			void SetPoints(const std::vector<Vector3>& points) { this->points = points; }

		protected:
			Vector3 GetCurrentDestination() const { return currentDest; }

			void NextDestination() {
				if (index == 0)
					hasReachedEnd = false;
				if (index < (points.size() - 1) && !hasReachedEnd) {
					currentDest = points[index++];
				}
				else if (index > 0) {
					hasReachedEnd = true;
					currentDest = points[index--];
				}
			}

			bool GetIsWaiting() const { return isWaiting; }
			void SetIsWaiting(const bool isWaiting) { 
				this->isWaiting = isWaiting;
				if (!isWaiting)
					waitTime = 0.0f;
			}

		private:
			Vector3 currentDest;
			GameObject* chaseObject;

			std::vector<Vector3> points;
			int index;
			bool hasReachedEnd;

			float distToCurrentDest;
			float distToObject;

			bool isWaiting;
			float waitTime;

			void SetupStateMachine();
		};
	}
}