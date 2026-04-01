#pragma once

#include "ControllerEvent.h"

namespace Wankel {

	class ControllerAxisEvent : public ControllerEvent {
	public:
		int GetAxis() const { return m_Axis; }
		float GetValue() const { return m_Value; }

		EVENT_CLASS_CATEGORY(EventCategoryController | EventCategoryControllerStick)
	protected:
		ControllerAxisEvent(int controllerID, int axis, float value)
			: ControllerEvent(controllerID), m_Axis(axis), m_Value(value) {}

		int m_Axis;
		float m_Value;
	};

	class ControllerAxisMovedEvent : public ControllerAxisEvent {
	public:
		ControllerAxisMovedEvent(int controllerID, int axis, float value)
			: ControllerAxisEvent(controllerID, axis, value) {}

		EVENT_CLASS_TYPE(ControllerStickMoved)

		std::string ToString() const override {
			return "ControllerAxisMovedEvent: Controller " +
				std::to_string(m_ControllerID) +
				", Axis " + std::to_string(m_Axis) +
				", Value " + std::to_string(m_Value);
		}
	};

}