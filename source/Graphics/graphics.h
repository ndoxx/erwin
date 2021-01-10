#pragma once

/*
 * Public API for the graphics library
 */

/* -------- WINDOW -------- */
#include "window.h"

/* -------- DATA -------- */

// Pipeline state
// Contains properties for each rendering stage

// Resources
// Interfaces to buffers and textures

// Resource views
// Describe how resources should be interpreted

// Shader resource bindings
// Describe all resources needed by a particular shader

// Render pass
// Contains all state needed for performing a sequence of drawing operations


/* -------- LOW LEVEL API -------- */

// Render device
// Create all graphics objects
#include "render_device.h"

// Device context
// Register render commands
#include "device_context.h"

// Swap chain
// Show the final rendered image on screen


/* -------- HIGH LEVEL API -------- */

// Renderer
// Combines previous low-level components
// Provides functions for frequently used pass configurations and drawing commands
// Allows for 2D batching
