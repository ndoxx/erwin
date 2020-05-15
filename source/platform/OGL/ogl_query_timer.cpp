#include "platform/OGL/ogl_query_timer.h"
#include "glad/glad.h"

#include <utility>

namespace erwin
{

OGLQueryTimer::OGLQueryTimer()
{
#ifndef USE_TIMESTAMP
    query_back_buffer_ = 0;
    query_front_buffer_ = 1;
    timer_ = 0;
#endif

    glGenQueries(2, &query_ID_[0]);

#ifndef USE_TIMESTAMP
    // dummy query to prevent OpenGL errors from popping out during first frame
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_front_buffer_]);
    glEndQuery(GL_TIME_ELAPSED);
#endif
}

void OGLQueryTimer::start(bool sync)
{
#ifndef USE_TIMESTAMP
    if(sync)
    {
        GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
    }
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_back_buffer_]);
#else
    (void)sync;
    glQueryCounter(query_ID_[0], GL_TIMESTAMP);
#endif
}

std::chrono::nanoseconds OGLQueryTimer::stop()
{
#ifndef USE_TIMESTAMP
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(query_ID_[query_front_buffer_], GL_QUERY_RESULT, static_cast<GLuint*>(&timer_));
    std::swap(query_back_buffer_, query_front_buffer_);

    return std::chrono::nanoseconds(timer_);
#else
    glQueryCounter(query_ID_[1], GL_TIMESTAMP);
    GLuint64 time_0, time_1;
    glGetQueryObjectui64v(query_ID_[0], GL_QUERY_RESULT , &time_0);
    glGetQueryObjectui64v(query_ID_[1], GL_QUERY_RESULT , &time_1);
    
    return std::chrono::nanoseconds(time_1-time_0);
#endif
}

} // namespace erwin