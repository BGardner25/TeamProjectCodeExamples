#pragma once
#include "../GameTech/GameTechRenderer.h"
#include <thread>
#include <mutex>

/*
*	Example Usage (the destructor MUST be called when finished, otherwise infinite loop): 
*
*	LoadingScreen* ls = new LoadingScreen();
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
				isLoading = true;
				loadCount = 0;
				textPos = Vector2(50, 50);
				loadingText = "LOADING.";
				// new thread runs Loading()
				loadingThread = std::thread([this] { this->Loading(); });
			}

			~LoadingScreen() {
				// indicate thread operation is complete
				isLoading = false;
				loadingThread.join();

				delete world;
				delete renderer;
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
			}

			void UpdateLoadingScreen() {
				glClearColor(0, 0, 0, 0);

				// simple animated loading bar - LOADING.(....)
				if (loadCount < 4) {
					loadingText += ".";
					loadCount++;
				}
				else {
					loadingText = "LOADING.";
					loadCount = 0;
				}

				renderer->DrawString(loadingText, textPos);
				renderer->RenderLoading();
			}

		private:
			int loadCount;
			bool isLoading;

			Vector2 textPos;
			string loadingText;

			std::thread loadingThread;
			std::mutex mMutex;

			GameWorld* world;
			GameTechRenderer* renderer;
		};
	}
}