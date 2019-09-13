#include "platform/ogl_query_timer.h"
#include "glad/glad.h"

namespace erwin
{

OGLQueryTimer::OGLQueryTimer():
query_back_buffer_(0),
query_front_buffer_(1)
{
    query_ID_ = new unsigned int[2];

    glGenQueries(1, &query_ID_[query_back_buffer_]);
    glGenQueries(1, &query_ID_[query_front_buffer_]);

    // dummy query to prevent OpenGL errors from popping out during first frame
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_front_buffer_]);
    glEndQuery(GL_TIME_ELAPSED);
}

OGLQueryTimer::~OGLQueryTimer()
{
    delete[] query_ID_;
}

void OGLQueryTimer::start()
{
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_back_buffer_]);
}

std::chrono::nanoseconds OGLQueryTimer::stop()
{
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(query_ID_[query_front_buffer_], GL_QUERY_RESULT, (GLuint*)&timer_);
    swap_query_buffers();

    return std::chrono::nanoseconds(timer_);
}

void OGLQueryTimer::swap_query_buffers()
{
    if(query_back_buffer_)
    {
        query_back_buffer_ = 0;
        query_front_buffer_ = 1;
    }
    else
    {
        query_back_buffer_ = 1;
        query_front_buffer_ = 0;
    }
}

} // namespace erwin