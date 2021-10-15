#pragma once
#include <iostream>
#include "QuadTree.h"
#include "Util.h"



class QTCollisions
{
public:
    QTCollisions(GLFWwindow* window, int width, int height, int pointCount) : w_width(width)
		, w_height(height)
		, nrPoints(pointCount)
        , window(window)
	{}

	void Setup()
	{
        this->treeLevelCapacity = QTCAPACITY;
        Rectangle boundary(w_width / 2, w_height / 2, w_width / 2, w_height / 2);

        qt = new QuadTree(boundary, this->treeLevelCapacity);

        points.resize(nrPoints);
        velocities.resize(nrPoints);
        speeds.resize(nrPoints);
        colors.resize(nrPoints);

        for (int i = 0; i < nrPoints; i++)
        {
            Point p;
            p.pos = glm::vec2(RandomFloat(w_width - 100 - w_width / 2, w_width / 2 + 100), RandomFloat(0, w_height));
            p.vel = glm::vec2(RandomFloat(-0.2f, 0.2f), RandomFloat(-0.2f, 0.2f));
            p.col = glm::vec4(RandomFloat(0.2f, 0.5f), RandomFloat(0.2f, 0.5f), RandomFloat(0.2f, 0.5f), 1.f);
            p.visited = false;
            p.radius = 1.0f;
            p.indexGlobal = i;

         //   qt->insert(p);
            qt->insert(p.pos, p.vel, p.acc, p.col, i, false);
        }
        this->oldState_left = GLFW_RELEASE;
        this->oldState_right = GLFW_RELEASE;
        this->oldWrapState = GLFW_RELEASE;

        this->xDrag = 0;
        this->yDrag = 0;
        this->xw = 0;
        this->yh = 0;

        this->avgCollisions = 0;
        this->countFFF = 0;

        this->press = false;
        this->doSearch = false;
        this->runNaive = false;
        this->runQuad = false;
        this->wrap = true;
        this->pauseTime = false;

        // opengl stuff
        Shader shader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
        program = shader.createProgram();

        //positions = GetQTPoints();
        positions.resize(nrPoints);
        colors.resize(nrPoints);

        //positions.clear();
        //colors.clear();

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
	}

    static bool collide(Point& p1, Point& p2)
    {
        if (!p2.visited && p1.indexGlobal != p2.indexGlobal)
        {
            float dx = p1.pos.x - p2.pos.x;
            float dy = p1.pos.y - p2.pos.y;
            float distance = dx * dx + dy * dy;
            if (distance < p1.radius + p2.radius)
            {
                glm::vec2 diff = p1.pos - p2.pos;
                p1.vel = diff * 0.1f;
                p2.vel = -diff * 0.1f;
                p1.visited = true;
                p2.visited = true;
                return true;
            }
        }
        return false;
    }


    void Run()
    {
        UserInputs();
        runQuad = true;
        /*
        if (!pauseTime)
        {
        
           float xMin = 10.f;
           float yMin = 10.f;
           float xMax = w_width - xMin;
           float yMax = w_height - yMin;
           qt->update([&](QuadTree& cell) -> void
               {
                   for (int i = 0; i < cell.nextFree; i++)
                   {
                       Point& p1 = cell.arrayPoints[i];

                       if (!p1.visited && cell.parent)
                       {
                           cell.arrayPoints[i].col.y -= 0.01;
                           cell.arrayPoints[i].col.z -= 0.001;
                           cell.arrayPoints[i].col.w -= 0.01;
                           cell.arrayPoints[i].col.x = std::max(cell.arrayPoints[i].col.x, 0.5f);
                           cell.arrayPoints[i].col.w = std::max(cell.arrayPoints[i].col.w, 0.3f);

                           cell.qVisited = false;
                           Rectangle searchArea(p1.pos.x, p1.pos.y, 8.0f, 8.0f);

                           float distance;
                           glm::vec2 diff;
                           cell.parent->fastQuery(searchArea, p1, [&](Point& p2) -> void
                               {
                                   if (!p2.visited && p1.indexGlobal != p2.indexGlobal)
                                   {
                                       diff = p1.pos - p2.pos;      
                                       distance = diff.x * diff.x + diff.y * diff.y;
                                       if (distance < p1.radius + p2.radius)
                                       {
                                           p1.vel = diff * 0.1f;
                                           p2.vel = -p1.vel;
                                           p1.visited = p2.visited = true;
                                       }
                                   }
                               
                               });
                           if (p1.visited)
                               cell.arrayPoints[i].col = glm::vec4(1, 0.7, 0, 1);
                       }
                       p1.visited = true;

                       p1.pos += p1.vel*4.0f;

                       // mirror
                       p1.pos.x = (p1.pos.x > xMax) ? xMin : ( (p1.pos.x < xMin) ? xMax : p1.pos.x );
                       p1.pos.y = (p1.pos.y > yMax) ? yMin : ( (p1.pos.y < yMin) ? yMax : p1.pos.y );

                       // wrap
                       //if (p1.pos.x < xMin)
                       //    p1.vel = glm::reflect(p1.vel, glm::vec2(1, 0));
                       //else if (p1.pos.x > xMax)
                       //    p1.vel = glm::reflect(p1.vel, glm::vec2(-1, 0));
                       //if (p1.pos.y < yMin)
                       //    p1.vel = glm::reflect(p1.vel, glm::vec2(0, 1));
                       //else if (p1.pos.y > yMax)
                       //    p1.vel = glm::reflect(p1.vel, glm::vec2(0, -1));

                   }
                   cell.qVisited = true;

               });

           // if (frameCount % 100 == 0)
            {
               qt->merge();
               qt->removeStale();
            }

             frameCount++;
        }
        */
        Draw();
    }
    void Draw()
    {
        glUseProgram(program);
        glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);

        //positions.clear();
        //colors.clear();
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
    
        if (runQuad && lines.size())
        {
            glDisableVertexAttribArray(1);
            glVertexAttrib4f(1, 0.15f, 0.15f, 0.15f, 1.f);

            GLuint vboLines;
            glGenBuffers(1, &vboLines);
            glBindBuffer(GL_ARRAY_BUFFER, vboLines);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * lines.size(), &lines[0].x, GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, vboLines);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glDrawArrays(GL_LINES, 0, lines.size());
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDeleteBuffers(1, &vboLines);
        }
       
        /// ------------------------------------ DEBUG SEARCH FUNCTION ------------------------------------ /// 
        /// 
        Rectangle search(xDrag, yDrag, xw, yh);
        
        //if (doSearch)
        {
            std::vector<Point> foundPoints;
           // qt->query(search, &foundPoints);
            //g_count = 0;
            doSearch = false;
        
            for (int i = 0; i < foundPoints.size(); i++)
            {
                foundPositions.push_back(foundPoints[i].pos);
            }
        }

        if (foundPositions.size())
        {
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vboPositions);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * foundPositions.size(), &foundPositions[0].x, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);

            glPointSize(2.0f);
            glDrawArrays(GL_POINTS, 0, foundPositions.size());
            glDisableVertexAttribArray(0);

            int x = search.x;
            int y = search.y;
            int w = search.w;
            int h = search.h;
            rangeLines.push_back({ x - w, y - h });
            rangeLines.push_back({ x - w, y + h });
            rangeLines.push_back({ x + w, y + h });
            rangeLines.push_back({ x + w, y - h });
            rangeLines.push_back({ x - w, y - h });

            glEnableVertexAttribArray(0);
            glVertexAttrib4f(1, 1.f, 1.f, 1.f, 1.f);

            GLuint vboRange;
            glGenBuffers(1, &vboRange);
            glBindBuffer(GL_ARRAY_BUFFER, vboRange);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * rangeLines.size(), &rangeLines[0].x, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, vboRange);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glDrawArrays(GL_LINE_STRIP, 0, rangeLines.size());
            glDisableVertexAttribArray(0);
            rangeLines.clear();
            glDeleteBuffers(1, &vboRange);
        }
        foundPositions.clear();
    }


    void UserInputs()
    {
        // left mouse
       //int newState_left = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT); /* Deprecated */
       //if (newState_left == GLFW_PRESS && oldState_left != newState_left)
       //{
       //    // TODO fix this 
       //
       //    double xpos, ypos;
       //    glfwGetCursorPos(window, &xpos, &ypos);
       //    Point p1(xpos, w_height - ypos, velocities.size()-1);
       //    p1.radius = 2.f;
       //  
       //    velocities.push_back(glm::vec2(-1, 0));
       //
       //    p1.vel = glm::vec2(-1, 0);
       //    p1.col = glm::vec4(1);
       //    //qt->insert(p1);
       //    qt->insert(p1.pos, p1.vel, p1.acc, p1.col);
       //    //radiuses.push_back(4.0f);//RandomFloat(2, 10);
       //    speeds.push_back(RandomFloat(1, 2));
       //    colors.push_back(glm::vec4(1, 0.8f, 0.f, 1.f));
       //    
       //    clicks++;
       //
       //}
       //oldState_left = newState_left;

        // right mouse
        int newState_right = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (newState_right == GLFW_PRESS)
        {
            if (!press)
            {
                rangeLines.clear();
                glfwGetCursorPos(window, &xDrag, &yDrag);
                yDrag = w_height - yDrag;
                press = true;
            }

            double xp, yp;
            glfwGetCursorPos(window, &xp, &yp);
            yp = w_height - yp;

            xw = std::abs(xDrag - xp);
            yh = std::abs(yDrag - yp);
        }
        if (newState_right == GLFW_RELEASE && oldState_right == GLFW_PRESS)
        {
            press = false;
            doSearch = true;
        }

        oldState_right = newState_right;

        int capacityState = glfwGetKey(window, GLFW_KEY_KP_ADD);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity++;
        }

        capacityState = glfwGetKey(window, GLFW_KEY_KP_SUBTRACT);
        if (capacityState == GLFW_PRESS)
        {
            treeLevelCapacity--;
            if (treeLevelCapacity <= 0)
                treeLevelCapacity = 1;
        }

        int wrapState = glfwGetKey(window, GLFW_KEY_W);
        if (wrapState == GLFW_RELEASE && oldWrapState == GLFW_PRESS)
        {
            wrap = !wrap;
        }
        oldWrapState = wrapState;

        int timeState = glfwGetKey(window, GLFW_KEY_SPACE);
        if (timeState == GLFW_RELEASE && oldTimeState == GLFW_PRESS)
        {
            pauseTime = !pauseTime;
            countFFF = 0;
        }
        oldTimeState = timeState;

        int state = glfwGetKey(window, GLFW_KEY_1);
        if (state == GLFW_PRESS)
        {
            runNaive = false;
            runQuad = false;
        }
        state = glfwGetKey(window, GLFW_KEY_2);
        if (state == GLFW_PRESS)
        {
            runNaive = true;
            runQuad = false;
        }
        state = glfwGetKey(window, GLFW_KEY_3);
        if (state == GLFW_PRESS)
        {
            runQuad = true;
            runNaive = false;
        }
    }

    void NaiveCollision()
    {
        //if (runNaive)
        //{
        //    nrOfChecks = 0;
        //    // check collision naive
        //    for (int i = 0; i < points.size(); i++)
        //    {
        //        Point* p1 = &points[i];
        //
        //        colors[i].g += 0.05f;
        //        colors[i].g = std::min(0.8f, colors[i].g);
        //
        //        for (int j = 0; j < points.size(); j++)
        //        {
        //            nrOfChecks++;
        //
        //            Point* p2 = &points[j];
        //
        //            float dx = p1->pos.x - p2->pos.x;
        //            float dy = p1->pos.y - p2->pos.y;
        //
        //            float distance = sqrt(dx * dx + dy * dy);
        //
        //            if (i != j && distance * 2.0f < points[i].radius + points[j].radius)
        //            {
        //                velocities[i].x =  dx * speeds[i];
        //                velocities[i].y =  dy * speeds[i];
        //                velocities[j].x = -dx * speeds[j];
        //                velocities[j].y = -dy * speeds[j];
        //
        //                colors[i].g = 0.0f;
        //                colors[j].g = 0.0f;
        //            }
        //        }
        //    }
        //}
    }

    QuadTree* GetQuadTree()
    {
        return qt;
    }

    void DebugDraw()
    {
        // display avg number of queries
        if (!pauseTime)
        {
            avgCollisions += nrOfChecks;
            countFFF++;
            if (countFFF >= 10)
            {
                countFFF = 0;
                std::cout << avgCollisions / 10 << std::endl;
                avgCollisions = 0;
            }
        }
    }

private:

	int w_width;
	int w_height;
	int nrPoints;
    int frameCount = 0;

    std::vector<Point*> points;
    std::vector<glm::vec2> velocities;
    std::vector<glm::vec4> colors;
    std::vector<float> speeds;

    // mostly for debugging
    std::vector<glm::vec2> foundPositions;
    std::vector<Point> failures{};

    // glfw input 
    int clicks;
    int oldState_left;
    int oldState_right;
    int oldWrapState;
    int oldTimeState;

    double xDrag, yDrag;
    double xw, yh;

    int avgCollisions;
    int countFFF;
    int nrOfChecks;

    bool press;
    bool doSearch;
    bool runNaive;
    bool runQuad;
    bool wrap;
    bool pauseTime;

    int treeLevelCapacity;

    GLFWwindow* window;
    QuadTree* qt;

    // opengl and draw
    GLuint vboPositions;
    GLuint vboColors;
    GLuint vboLines;
    GLuint vboFoundPoints;
    GLuint vboRange;

    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> lines;
    std::vector<glm::vec2> rangeLines;
    glm::mat4 mvp;
    GLuint mvpId;

    GLuint program;
};