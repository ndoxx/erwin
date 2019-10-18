#include "platform/ogl_query_timer.h"
#include "glad/glad.h"

#include <utility>

namespace erwin
{

OGLQueryTimer::OGLQueryTimer():
query_back_buffer_(0),
query_front_buffer_(1)
{
    glGenQueries(1, &query_ID_[query_back_buffer_]);
    glGenQueries(1, &query_ID_[query_front_buffer_]);

    // dummy query to prevent OpenGL errors from popping out during first frame
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_front_buffer_]);
    glEndQuery(GL_TIME_ELAPSED);
}

void OGLQueryTimer::start()
{
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_back_buffer_]);
}

std::chrono::nanoseconds OGLQueryTimer::stop()
{
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(query_ID_[query_front_buffer_], GL_QUERY_RESULT, (GLuint*)&timer_);
    std::swap(query_back_buffer_, query_front_buffer_);

    return std::chrono::nanoseconds(timer_);
}

} // namespace erwin