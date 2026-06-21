#pragma once

struct RendererCapabilities
{
    bool SupportsMeshShaders = false;
    bool SupportsWorkGraphs = false;
    bool SupportsExecuteIndirect = false;
    bool SupportsTimestampQueries = false;
};