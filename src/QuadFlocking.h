#pragma once
#include <iostream>
#include "QuadTree.h"
#include "Util.h"
#include <algorithm>
#include <functional>
#include <thread>
#include <future>

class QTFlock
{
public:
    QTFlock(GLFWwindow* window, int width, int height, int pointCount) : w_width(width)
        , w_height(height)
        , nrPoints(pointCount)
        , window(window)
    {}

    void Setup()
    {
        Rectangle boundary(w_width / 2, w_height / 2, w_width / 2, w_height / 2);
        qt = new QuadTree(boundary, QTCAPACITY);

        colors.resize(nrPoints);

        for (int i = 0; i < nrPoints; i++)
        {
            Point p;
            p.acc = glm::vec2(RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f));
            p.pos = glm::vec2(RandomFloat(0, w_width), RandomFloat(0, w_height));
            p.vel = glm::vec2(RandomFloat(-0.2f, 0.2f), RandomFloat(-0.2f, 0.2f));
            p.col = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
            p.visited = false;
            p.radius = 1.0f;
            p.indexGlobal = i;

            //qt->insert(p);
            qt->insert(p.pos, p.vel, p.acc, p.col, p.indexGlobal, false);
        }
  
        // opengl stuff
        Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
        program = shader.createProgram();

        positions.resize(nrPoints);
        colors.resize(nrPoints);

        int idx = 0;
        qt->getAllPositions(positions, colors, idx);

        glGenBuffers(1, &vboPositions);
        glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &vboColors);
        glBindBuffer(GL_ARRAY_BUFFER, vboColors);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

        GLuint vboLines;
        glGenBuffers(1, &vboLines);

        std::vector<glm::vec2> lines;
        qt->GetBoundsLineSegments(lines);

        mvp = glm::ortho(0.f, static_cast<float>(w_width), 0.f, static_cast<float>(w_height), 0.f, 100.f);
        mvpId = glGetUniformLocation(program, "MVP");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        boidParams.xMin = 10.0f;
        boidParams.yMin = 10.0f;
        boidParams.xMax = w_width - 10.f;
        boidParams.yMax = w_height - 10.f;
    }

    void updateExt(QuadTree* node, BoidParameters& params)
    {
        int i;
        for (i = 0; i < 4; ++i)
        {
            if (node && node->quadrants[i]) // if has quadtrant i 
            {
                updateExt(node->quadrants[i], params);
            }
        }
        node->updateBoids(params);
        //updateFunction(*this);

        QuadTree* current = node;
        for (i = node->nextFree - 1; i >= 0; --i)
        {
            if (!node->boundary.contains(node->arrayPos[i]))
            {
                int indexToRemove = node->nextFree - 1;
                std::swap(node->arrayPos[i], node->arrayPos[indexToRemove]);
                std::swap(node->arrayVel[i], node->arrayVel[indexToRemove]);
                std::swap(node->arrayAcc[i], node->arrayAcc[indexToRemove]);
                std::swap(node->arrayCol[i], node->arrayCol[indexToRemove]);
                std::swap(node->arrayIdx[i], node->arrayIdx[indexToRemove]);
                std::swap(node->arrayVis[i], node->arrayVis[indexToRemove]);
        
                while (!current->boundary.contains(node->arrayPos[indexToRemove]))
                {
                    if (current->parent != nullptr)
                        current = current->parent;
                    else
                        break; // if root node, the leave
                }
        
                if (!current->insert(node->arrayPos[indexToRemove],
                    node->arrayVel[indexToRemove],
                    node->arrayAcc[indexToRemove],
                    node->arrayCol[indexToRemove],
                    node->arrayIdx[indexToRemove],
                    node->arrayVis[indexToRemove]))
                {
                    if (isnan(node->arrayPos[indexToRemove].x))
                    {
                        node->arrayPos[i] = glm::vec2(0);
                    }
                }
                node->nextFree--;
            }
        }
    }


    void Run()
    {
        if (!pauseTime)
        {           
            boidParams.alignmentMaxForce = alignmentMaxForce;
            boidParams.alignmentMaxSpeed = alignmentMaxSpeed;
            boidParams.alignmentOn = alignmentOn;
            boidParams.alignmentRadius = alignmentRadius;

            boidParams.cohesionMaxForce = cohesionMaxForce;
            boidParams.cohesionMaxSpeed = cohesionMaxSpeed;
            boidParams.cohesionOn = cohesionOn;
            boidParams.cohesionRadius = cohesionRadius;

            boidParams.separationMaxForce = separationMaxForce;
            boidParams.separationMaxSpeed = separationMaxSpeed;
            boidParams.separationOn = separationOn;
            boidParams.separationRadius = separationRadius;
            
            boidParams.queryRadius = queryRadius;

            boidParams.alignment *= 0.f;
            boidParams.cohesion *= 0.f;
            boidParams.separation *= 0.f;
            boidParams.aCount = 0;
            boidParams.cCount = 0;
            boidParams.sCount = 0;            

            updateExt(qt, boidParams);

            qt->updateVelocity();
            qt->merge();
            qt->removeStale();
            frameCount++;
        }
    }
    void Draw()
    {
        glUseProgram(program);
        glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);

        int idx = 0;
        qt->getAllPositions(positions, colors, idx);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, vboColors);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glPointSize(2.0f);
        glDrawArrays(GL_POINTS, 0, positions.size());

        lines.clear();
        qt->GetBoundsLineSegments(lines);

        if (lines.size())
        {
            glDisableVertexAttribArray(1);
            glVertexAttrib4f(1, 0.10f, 0.10f, 0.10f, 1.f);

            GLuint vboLines;
            glGenBuffers(1, &vboLines);
            glBindBuffer(GL_ARRAY_BUFFER, vboLines);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * lines.size(), &lines[0].x, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, vboLines);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glDrawArrays(GL_LINES, 0, lines.size());
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDeleteBuffers(1, &vboLines);
        }
    }

    void ImGuiMenu()
    {
        static float qR = 2.0f;

        static float aR = 150.0f;
        static float cR = 4.0f;
        static float sR = 30.0f;

        static float aMaxSpeed = 11.f;
        static float aMaxForce = 1.2f;

        static float cMaxSpeed = 20.0f;
        static float cMaxForce = 4.2f;

        static float sMaxSpeed = 20.f;
        static float sMaxForce = 1.6f;

        static int counter = 0;
        static int treeCapacity = 10;

        static float maxSearchRadius = 500.f;

        ImVec4 col_text = ImColor(255, 255, 255);
        ImVec4 col_main = ImColor(30, 30, 30);
        ImVec4 col_oran = ImColor(255, 185, 0);
        ImVec4 col_doran = ImColor(255, 150, 0);
        ImVec4 col_area = ImColor(51, 56, 69);
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_FrameBg] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
        style.Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(col_doran.x, col_doran.y, col_doran.z, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 0.90f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(col_oran.x, col_oran.y, col_oran.z, 1.00f);

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoBackground;
        //ImGui::Begin("Dear ImGui Demo", p_open, window_flags)
        ImGui::Begin("Flocking control", nullptr, window_flags);
        style.Colors[ImGuiCol_Text] = ImVec4(255, 255, 255, 1.00f);

        ImGui::SliderFloat("Query Radius", &qR, 0.0f, maxSearchRadius);
        this->queryRadius = qR;

        ImGui::Checkbox("Alignment", &alignmentOn);
        ImGui::SliderFloat("radius_A", &aR, 0.0f, qR);
        this->alignmentRadius = aR;
        ImGui::SliderFloat("max speed_A", &aMaxSpeed, 0.0f, 100.0f);
        this->alignmentMaxSpeed = aMaxSpeed;
        ImGui::SliderFloat("max force_A", &aMaxForce, 0.0f, 20.0f);
        this->alignmentMaxForce = aMaxForce;



        ImGui::Checkbox("Cohesion", &cohesionOn);
        ImGui::SliderFloat("radius_C", &cR, 0.0f, maxSearchRadius);
        this->cohesionRadius = cR;
        ImGui::SliderFloat("max speed_C", &cMaxSpeed, 0.0f, 100.0f);
        this->cohesionMaxSpeed = cMaxSpeed;
        ImGui::SliderFloat("max force_C", &cMaxForce, 0.0f, 20.0f);
        this->cohesionMaxForce = aMaxForce;



        ImGui::Checkbox("Separation", &separationOn);
        ImGui::SliderFloat("radius_S", &sR, 0.0f, maxSearchRadius);
        this->separationRadius = sR;
        ImGui::SliderFloat("max speed_S", &sMaxSpeed, 0.0f, 100.0f);
        this->separationMaxSpeed = sMaxSpeed;
        ImGui::SliderFloat("max force_S", &sMaxForce, 0.0f, 20.0f);
        this->separationMaxForce = sMaxForce;

        //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        style.Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1.00f);
        if (ImGui::Button("Reset"))
        {
            Setup();
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

private:
    /// boid members
    float queryRadius;

    float alignmentRadius;
    float cohesionRadius;
    float separationRadius;

    // max values
    float alignmentMaxForce;
    float alignmentMaxSpeed;

    float cohesionMaxForce;
    float cohesionMaxSpeed;

    float separationMaxForce;
    float separationMaxSpeed;
    bool alignmentOn = true;
    bool cohesionOn = true;
    bool separationOn = true;
    /// boid members end

    int w_width;
    int w_height;
    int nrPoints;
    int frameCount = 0;

    std::vector<glm::vec2> positions;
    std::vector<glm::vec4> colors;

    bool wrap;
    bool pauseTime;
    GLFWwindow* window;
    QuadTree* qt;

    // opengl and draw
    GLuint vboPositions;
    GLuint vboColors;
    GLuint vboLines;
    GLuint vboFoundPoints;
    GLuint vboRange;

    std::vector<glm::vec2> lines;
    std::vector<glm::vec2> rangeLines;
    glm::mat4 mvp;
    GLuint mvpId;

    GLuint program;

    BoidParameters boidParams;
};