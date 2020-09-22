#pragma once
#include "../GameTech/GameTechRenderer.h"
#include <thread>
#include <mutex>

/*
*	@Author - Bailey Gardner
*	LoadingScreen class used to display a loading screen while assets are loaded in the background.
*	Example Usage (the destructor MUST be called when finished, otherwise infinite loop): 
*
*	LoadingScreen ls = new LoadingScreen();
*
*	// anything that needs to load goes here
*
*	delete ls;
*	ls = nullptr;
*/

namespace NCL {
	namespace CSC8503 {
		class LoadingScreen {
		public:
			// MUST call destructor after loading assets
			LoadingScreen() : mMutex() {
				loadingThread = std::thread([this] {this->Loading(); });
				isLoading = true;
			}

			~LoadingScreen() {
				// indicate thread operation is complete
				isLoading = false;
				loadingThread.join();

				delete world;
				world = nullptr;
				if (renderer) {
					delete renderer;
					renderer = nullptr;
				}
			}

			void Loading() {
				std::lock_guard<std::mutex> guard(mMutex);
				// since it's a new thread, we need a new renderer
				world = new GameWorld(); 
				renderer = new GameTechRenderer(*world);
				// display black screen
				world->GetMainCamera()->SetCameraType(CameraType::Orthographic);
				glClearColor(0, 0, 0, 0);

				while (isLoading) {
					UpdateLoadingScreen();
					Sleep(600);
				}

				delete renderer;
				renderer = nullptr;
			}

			void UpdateLoadingScreen() {
				glClearColor(0, 0, 0, 0);

				// very simple animated loading bar - LOADING...
				if (loadCount < 4) {
					loadingText += ".";
					renderer->DrawString(loadingText, textPos);
					loadCount++;
				}
				else {
					loadingText = "Loading.";
					renderer->DrawString(loadingText, textPos);
					loadCount = 0;
				}
				
				renderer->Render();
			}

		private:
			int loadCount = 0;
			bool isLoading;

			Vector2 textPos = Vector2(50, 50);
			string loadingText = "Loading";

			std::thread loadingThread;
			std::mutex mMutex;

			GameWorld* world;
			GameTechRenderer* renderer;
		};
	}
}