#pragma once

#include "ControllerEvent.h"

namespace Wankel {

	class ControllerConnectedEvent : public ControllerEvent {
	public:
		ControllerConnectedEvent(int controllerID)
			: ControllerEvent(controllerID) {}

		EVENT_CLASS_TYPE(ControllerButtonPressed) // ⚠️ fix enum if you add new type

		std::string ToString() const override {
			return "ControllerConnectedEvent: Controller " +
				std::to_string(m_ControllerID);
		}
	};

	class ControllerDisconnectedEvent : public ControllerEvent {
	public:
		ControllerDisconnectedEvent(int controllerID)
			: ControllerEvent(controllerID) {}

		EVENT_CLASS_TYPE(ControllerButtonReleased) // ⚠️ fix enum

		std::string ToString() const override {
			return "ControllerDisconnectedEvent: Controller " +
				std::to_string(m_ControllerID);
		}
	};

}