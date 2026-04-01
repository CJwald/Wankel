#pragma once

#include "ControllerEvent.h"

namespace Wankel {

	class ControllerButtonEvent : public ControllerEvent {
	public:
		int GetButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryController | EventCategoryControllerButton)
	protected:
		ControllerButtonEvent(int controllerID, int button)
			: ControllerEvent(controllerID), m_Button(button) {}

		int m_Button;
	};

	class ControllerButtonPressedEvent : public ControllerButtonEvent {
	public:
		ControllerButtonPressedEvent(int controllerID, int button)
			: ControllerButtonEvent(controllerID, button) {}

		EVENT_CLASS_TYPE(ControllerButtonPressed)

		std::string ToString() const override {
			return "ControllerButtonPressedEvent: Controller " +
				std::to_string(m_ControllerID) + ", Button " +
				std::to_string(m_Button);
		}
	};

	class ControllerButtonReleasedEvent : public ControllerButtonEvent {
	public:
		ControllerButtonReleasedEvent(int controllerID, int button)
			: ControllerButtonEvent(controllerID, button) {}

		EVENT_CLASS_TYPE(ControllerButtonReleased)

		std::string ToString() const override {
			return "ControllerButtonReleasedEvent: Controller " +
				std::to_string(m_ControllerID) + ", Button " +
				std::to_string(m_Button);
		}
	};

}