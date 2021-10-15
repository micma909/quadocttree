#ifndef __Shader__
#define __Shader__
#include <GL/glew.h>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader{
private:
	GLuint glProgram;
    
    std::string v_source;
	std::string f_source;
    std::string c_source;
    bool hasComputeShader;
    //chached uniforms
    std::map<std::string,GLint> uniforms,attributes;
    
protected:
	std::string source(const std::string &src_file);
	void printShaderInfoLog(GLuint obj);
	void printProgramInfoLog(GLuint obj);
    GLuint addShader(GLenum shaderType,const std::string &source);
    void deleteShader(GLuint shader);
    void checkCompileStatus(GLuint& shader);
public:
    Shader(const std::string &v, const std::string &f);
    Shader(const std::string &v, const std::string &f, const std::string &c);
    GLuint createProgram();
	GLuint program()  { return glProgram;}
    
    GLint uniform(const std::string &name);
    GLint attribute(const std::string &name);

    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void set3Float(const std::string& name, float v1, float v2, float v3);
    void set3Float(const std::string& name, glm::vec3 v);
    void set4Float(const std::string& name, float v1, float v2, float v3, float v4);
    void set4Float(const std::string& name, glm::vec4 v);
    void setMat4(const std::string& name, glm::mat4 val);

};
#endif