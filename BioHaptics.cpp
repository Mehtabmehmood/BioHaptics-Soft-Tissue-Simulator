#include <iostream>
#include "chai3d.h"
#include <GLFW/glfw3.h>
#include <cmath>

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    // Use OpenGL 2.1, as in the CHAI3D examples
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // Create our BioHaptics window
    GLFWwindow* window = glfwCreateWindow(
        1000, 700,
        "BioHaptics - Virtual Tissue Simulator",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

#ifdef GLEW_VERSION
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
#endif
    // Create the CHAI3D virtual world
    chai3d::cWorld* world = new chai3d::cWorld();
    world->m_backgroundColor.setBlack();

    // Create a camera
    chai3d::cCamera* camera = new chai3d::cCamera(world);
    world->addChild(camera);

    camera->set(
        chai3d::cVector3d(2.5, 2.5, 1.5),
        chai3d::cVector3d(0.0, 0.0, 0.0),
        chai3d::cVector3d(0.0, 0.0, 1.0)
    );
    // Create a mesh for future soft-tissue deformation
    chai3d::cMesh* tissueMesh = new chai3d::cMesh();
    world->addChild(tissueMesh);
    // Create a grid of vertices for the tissue surface
    const int gridSize = 20;
    const double spacing = 0.06;

    for (int i = 0; i <= gridSize; i++)
    {
        for (int j = 0; j <= gridSize; j++)
        {
            double y = (i - gridSize / 2) * spacing;
            double z = (j - gridSize / 2) * spacing;

            tissueMesh->newVertex(-0.6, y, z);
        }
    }

    // Connect neighbouring vertices into triangles
    for (int i = 0; i < gridSize; i++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            int v0 = i * (gridSize + 1) + j;
            int v1 = (i + 1) * (gridSize + 1) + j;
            int v2 = v1 + 1;
            int v3 = v0 + 1;

            tissueMesh->newTriangle(v0, v1, v2);
            tissueMesh->newTriangle(v0, v2, v3);
        }
    }

    // Give the tissue surface a pink colour
    tissueMesh->m_material->m_diffuse.set(0.9f, 0.4f, 0.5f);

    // Enable material rendering
    tissueMesh->setUseMaterial(true);

    // Display the triangle edges
    tissueMesh->setWireMode(false);

    // Calculate surface normals for lighting
    tissueMesh->computeAllNormals();

    // Black wireframe overlay
    chai3d::cMesh* wireMesh = new chai3d::cMesh();
    world->addChild(wireMesh);

    // Copy the tissue mesh vertices
    for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
    {
        wireMesh->newVertex(tissueMesh->m_vertices->getLocalPos(i));
    }

    // Copy the same triangular grid
    for (int i = 0; i < gridSize; i++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            int v0 = i * (gridSize + 1) + j;
            int v1 = (i + 1) * (gridSize + 1) + j;
            int v2 = v1 + 1;
            int v3 = v0 + 1;

            wireMesh->newTriangle(v0, v1, v2);
            wireMesh->newTriangle(v0, v2, v3);
        }
    }

    // Move the black grid slightly in front of the pink surface
    wireMesh->setLocalPos(0.008, 0.0, 0.0);

    wireMesh->setWireMode(true);
    wireMesh->setUseMaterial(true);

    wireMesh->m_material->m_ambient.setBlack();
    wireMesh->m_material->m_diffuse.setBlack();
    wireMesh->m_material->m_specular.setBlack();
    // Create a pink sphere
    chai3d::cShapeSphere* tissue = new chai3d::cShapeSphere(0.5);
    world->addChild(tissue);
    tissue->m_material->m_diffuse.set(0.9f, 0.4f, 0.5f);
    

        // Create a virtual palpation probe
        chai3d::cShapeSphere* probe = new chai3d::cShapeSphere(0.08);
    world->addChild(probe);

    // Position the probe in front of the tissue
    probe->setLocalPos(0.8, 0.0, 0.0);

    // Give the probe a blue colour
    probe->m_material->m_diffuse.set(0.1f, 0.5f, 1.0f);
    

    // Create lighting
    chai3d::cDirectionalLight* light =
        new chai3d::cDirectionalLight(world);

    world->addChild(light);
    light->setEnabled(true);
    light->setDir(-1.0, 0.0, -1.0);

    // Create the CHAI3D viewport
    chai3d::cViewport* viewport = new chai3d::cViewport(camera);

    // Controls how quickly the tissue returns to its original shape
    const double recoverySpeed = 0.08;

    // Previous filtered force value
    double smoothedForce = 0.0;
    // Variables for calculating penetration velocity
    double previousPenetration = 0.0;
    double previousTime = glfwGetTime();
    double penetrationVelocity = 0.0;
    // Main application loop
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        // Measure time elapsed since the previous frame
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        // Read mouse position
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Get window dimensions
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // Move probe towards or away from tissue
        if (windowHeight > 0)
        {
            double normalizedY = mouseY / windowHeight;

            // Top: farther from tissue
            // Bottom: closer to tissue
            double probeX = 1.8 - normalizedY * 1.6;

            // Keep the virtual probe within a safe simulation range
            probeX = chai3d::cClamp(probeX, 0.55, 1.8);

            probe->setLocalPos(probeX, 0.0, 0.0);

            // Tissue contact parameters
            const double tissueRadius = 0.5;
            const double probeRadius = 0.08;

            // Tissue stiffness: lower = softer, higher = harder
            const double tissueStiffness = 15.0;
            // Tissue damping coefficient
            const double tissueDamping = 0.5;

            // Calculate probe penetration into the flat square tissue surface
            const double tissueSurfaceX = 0.6;

            double penetration =
                chai3d::cMax(0.0, tissueSurfaceX + probeRadius - probeX);
            // Calculate how quickly penetration is changing
            if (deltaTime > 0.000001)
            {
                penetrationVelocity =
                    (penetration - previousPenetration) / deltaTime;
            }
            else
            {
                penetrationVelocity = 0.0;
            }

            previousPenetration = penetration;
            // Calculate simulated reaction force
            double contactForce = 0.0;

            // Contact detection code
            if (penetration > 0.0)
            {
                // Spring force: resistance due to penetration depth
                double springForce = tissueStiffness * penetration;

                // Damping force: resistance due to compression speed
                double dampingForce = tissueDamping * penetrationVelocity;

                // Total simulated contact force
                contactForce = chai3d::cMax(0.0,springForce + dampingForce);

                // Maximum simulated contact force
                const double maxContactForce = 3.0;

                // Limit the calculated force to a defined range
                contactForce = chai3d::cClamp(
                    contactForce,
                    0.0,
                    maxContactForce
                );

                // Deform the tissue mesh around its centre
                for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
                {
                    chai3d::cVector3d pos = tissueMesh->m_vertices->getLocalPos(i);

                    double y = pos.y();
                    double z = pos.z();

                    // Distance from the centre of the tissue surface
                    double r2 = y * y + z * z;

                    // Smooth, localised indentation
                    double influence = exp(-r2 / 0.08);

                    double indentation = chai3d::cClamp(
                        penetration * 2.0, 0.0, 0.2
                    );

                    tissueMesh->m_vertices->setLocalPos(
                        i,
                        chai3d::cVector3d(
                            0.6 - indentation * influence,
                            y,
                            z
                        )
                    );
                }

                tissueMesh->computeAllNormals();
                // Restore the black grid to match the pink tissue
                for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
                {
                    chai3d::cVector3d pos =
                        tissueMesh->m_vertices->getLocalPos(i);

                    wireMesh->m_vertices->setLocalPos(i, pos);
                }

                // Make the black grid follow the deformed pink tissue
                for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
                {
                    chai3d::cVector3d pos =
                        tissueMesh->m_vertices->getLocalPos(i);

                    wireMesh->m_vertices->setLocalPos(i, pos);
                }
                // Red tissue indicates contact
                tissue->m_material->m_diffuse.set(1.0f, 0.1f, 0.1f);
            }
            else
            {
                // Restore original tissue colour
                tissue->m_material->m_diffuse.set(0.9f, 0.4f, 0.5f);
                tissueMesh->m_material->m_diffuse.set(0.9f, 0.4f, 0.5f);
                    
                // Restore the flat tissue mesh
                for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
                {
                    chai3d::cVector3d pos = tissueMesh->m_vertices->getLocalPos(i);

                    // Gradually return each vertex to its original X position
                    double newX = pos.x() + recoverySpeed * (0.6 - pos.x());

                    tissueMesh->m_vertices->setLocalPos(
                        i,
                        chai3d::cVector3d(newX, pos.y(), pos.z())
                    );
                }

                tissueMesh->computeAllNormals();
                // Keep the black grid aligned with the restored pink surface
                for (unsigned int i = 0; i < tissueMesh->getNumVertices(); i++)
                {
                    chai3d::cVector3d pos =
                        tissueMesh->m_vertices->getLocalPos(i);

                    wireMesh->m_vertices->setLocalPos(i, pos);
                }
            }
            // Smooth the calculated contact force
            const double smoothingFactor = 0.15;

            smoothedForce += smoothingFactor *
                (contactForce - smoothedForce);

            // Display calculated force
            std::cout << "\rContact Force: "
                << smoothedForce << " (simulated force units)     "
                << std::flush;
        }

        // Get the actual framebuffer dimensions
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Render the CHAI3D world
        if (width > 0 && height > 0)
        {
            world->updateShadowMaps(false, false);
            viewport->renderView(width, height);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    delete viewport;
    delete world;

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}