#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "Shader.h"

Shader::Shader(const std::string &v, const std::string &f):hasComputeShader(false)
{
    v_source = source(v);
    f_source = source(f);
}

Shader::Shader(const std::string &v, const std::string &f, const std::string &c):Shader(v,f)
{
    hasComputeShader = true;
    c_source = source(c);
}

GLuint Shader::addShader(GLenum shaderType,const std::string &source)
{
    const GLchar* vert_source = (const GLchar *)source.c_str();
    GLuint shader   = glCreateShader(shaderType);
	glShaderSource(shader, 1, &vert_source, 0);
    glCompileShader(shader);
	glAttachShader(glProgram, shader);
    return shader;
}

void Shader::deleteShader(GLuint shader)
{
    glDetachShader(glProgram,shader);
    glDeleteShader(shader);
}

GLuint Shader::createProgram()
{
    glProgram = glCreateProgram();
	GLuint glVertexShader   = addShader(GL_VERTEX_SHADER,v_source);
	GLuint glFragmentShader = addShader(GL_FRAGMENT_SHADER,f_source);
    GLuint glComputeShader = 0;
    if(hasComputeShader) glComputeShader = addShader(GL_COMPUTE_SHADER,c_source);

    printShaderInfoLog(glVertexShader);
    printShaderInfoLog(glFragmentShader);
    if(hasComputeShader) printProgramInfoLog(glComputeShader);
    glLinkProgram(glProgram);
    deleteShader(glVertexShader);
    deleteShader(glFragmentShader);
    if(hasComputeShader) deleteShader(glComputeShader);

    return glProgram;
}

void Shader::printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
 
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        free(infoLog);
    }
}

void Shader::printProgramInfoLog(GLuint obj)
	{
	    int infologLength = 0;
	    int charsWritten  = 0;
	    char *infoLog;

		glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
	    if (infologLength > 0)
	    {
	        infoLog = (char *)malloc(infologLength);
	        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
			printf("%s\n",infoLog);
	        free(infoLog);
	    }
	}
 

void Shader::checkCompileStatus(GLuint& shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE){
		std::cout << "Shader did not compile properly" << std::endl;
		printShaderInfoLog(shader);
		return;
	}
	else{
		std::cout << "Shader compiled sucessfully" << std::endl; 
	}
}

//#include <windows.h>
//#include <string>
//#include <iostream>
//using namespace std;
//
//wstring ExePath() 
//{
//    TCHAR buffer[MAX_PATH] = { 0 };
//    GetModuleFileName(NULL, buffer, MAX_PATH);
//    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
//    return std::wstring(buffer).substr(0, pos);
//}

std::string Shader::source(const std::string &src_file)
{
    std::ifstream ifs;

    ifs.open(src_file.c_str());

    if (!ifs){
        std::stringstream ss;
        ss << "File " << src_file << " not found";
        throw std::runtime_error(ss.str());
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());

    return content;
}


GLint Shader::uniform(const std::string &name)
{
    
    std::map<std::string,GLint>::iterator it = uniforms.find(name);

    if(it != uniforms.end()) return it->second;
    
    GLint location = glGetUniformLocation(program(),name.c_str());
    uniforms.insert(std::pair<std::string, GLint>(name,location));
    return location;
}



GLint Shader::attribute(const std::string &name)
{
    std::map<std::string,GLint>::iterator it = attributes.find(name);
    
    if(it != attributes.end()) return it->second;
    
    GLint location = glGetAttribLocation(program(),name.c_str());
    attributes.insert(std::pair<std::string, GLint>(name,location));
    return location;
}

/*
    set uniform variables
*/

void Shader::setBool(const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(program(), name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(program(), name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(program(), name.c_str()), value);
}

void Shader::set3Float(const std::string& name, float v1, float v2, float v3) {
    glUniform3f(glGetUniformLocation(program(), name.c_str()), v1, v2, v3);
}

void Shader::set3Float(const std::string& name, glm::vec3 v) {
    glUniform3f(glGetUniformLocation(program(), name.c_str()), v.x, v.y, v.z);
}

void Shader::set4Float(const std::string& name, float v1, float v2, float v3, float v4) {
    glUniform4f(glGetUniformLocation(program(), name.c_str()), v1, v2, v3, v4);
}

void Shader::set4Float(const std::string& name, glm::vec4 v) {
    glUniform4f(glGetUniformLocation(program(), name.c_str()), v.x, v.y, v.z, v.w);
}

void Shader::setMat4(const std::string& name, glm::mat4 val) {
    glUniformMatrix4fv(glGetUniformLocation(program(), name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
}



















