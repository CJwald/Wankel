#pragma once

#include <Wankel/ECS/Scene.h>
#include <Wankel/ECS/Components.h>
#include <memory>

namespace Wankel {

	class Player {
	public:
		Player() = default;
    	void Create(Scene& scene);
    	Entity GetEntity() const { return m_Player; }
		Entity GetCamera() const { return m_Camera; }
	
	private:
	    void Build(Scene& scene);
	
	private:
	    Entity m_Player;
	
	    // optional: keep references if you want external access
	    Entity m_ShipLeft;
	    Entity m_ShipRight;
	    Entity m_GunLeft;
	    Entity m_GunRight;
	    Entity m_Camera;
	
	    std::shared_ptr<Mesh> m_ShipMesh;
	    std::shared_ptr<Mesh> m_ShipMeshMirrored;
	    std::shared_ptr<Mesh> m_GunMesh;
	};

}
