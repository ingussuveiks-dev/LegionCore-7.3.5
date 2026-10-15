#ifndef STORMHEIM_DRAGONS_ROUTE_H
#define STORMHEIM_DRAGONS_ROUTE_H
namespace StormheimDragons
{
Position const Return = {1977.46f,2551.44f,489.014f,5.49424f};
Position const Start = {1977.46f,2551.44f,492.014f,5.49424f};
// Preserve the installed summit arrival location; its native floor is676.926.
Position const Landing = {2082.43f,2643.01f,676.926f,1.42f};
Position const Route[] = {
    {1977.46f,2551.44f,520.0f,0.0f},
    {1940.0f,2520.0f,525.0f,0.0f},
    {1910.0f,2550.0f,590.0f,0.0f},
    {1910.0f,2620.0f,680.0f,0.0f},
    {1960.0f,2670.0f,720.0f,0.0f},
    {2040.0f,2670.0f,720.0f,0.0f},
    {2082.43f,2643.01f,700.0f,1.42f},
    {2082.43f,2643.01f,679.926f,1.42f}
};
}
#endif
