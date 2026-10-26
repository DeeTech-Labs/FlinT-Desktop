#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 resolution;
    vec4 ink;
    float kind;
    float phase;
    float sweep;
    float strength;
    float cornerRadius;
};
void main() {
    vec2 uv = qt_TexCoord0;
    vec3 color = ink.rgb;
    float alpha;
    if (kind < 0.5) {
        // A quiet three-lobed light contour. The center stays clear for the timer.
        vec2 p = (uv - 0.5) * vec2(resolution.x / max(resolution.y, 1.0) * 0.58, 1.0);
        float angle = atan(p.y, p.x);
        float radius = length(p);
        float breath = 0.5 + 0.5 * sin(phase * 0.78);
        float contour = 0.31 + 0.018 * sin(angle * 3.0 + phase * 0.13)
                             + 0.009 * cos(angle * 5.0 - phase * 0.09);
        float d = radius - contour - breath * 0.012;
        float haze = exp(-d*d*220.0) * (0.09 + breath*0.035);
        float filament = exp(-abs(d)*230.0) * 0.13;
        float falloff = 1.0 - smoothstep(0.38, 0.49, radius);
        color = mix(vec3(0.16, 0.33, 0.91), ink.rgb, 0.5 + 0.5*sin(angle + phase*0.10));
        alpha = (haze + filament) * falloff * strength;
    } else {
        // Selected-card edge. The sweep completes once; the idle rim is static.
        vec2 pixel = uv * resolution;
        float radius = min(cornerRadius, min(resolution.x, resolution.y)*0.5);
        vec2 q = abs(pixel - resolution*0.5) - resolution*0.5 + radius + 1.0;
        float distanceToEdge = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
        float edge = 1.0 - smoothstep(0.35, 1.5, abs(distanceToEdge));
        float inside = 1.0 - smoothstep(-0.5, 0.5, distanceToEdge);
        float glow = exp(-abs(distanceToEdge)*0.14) * inside;
        float beam = exp(-pow((uv.x + uv.y*0.3 - (sweep*1.75 - 0.2))/0.14, 2.0));
        float envelope = sin(clamp(sweep, 0.0, 1.0)*3.14159265);
        alpha = (edge*0.30 + glow*0.018 + beam*envelope*(edge*0.55 + glow*0.07)) * strength;
        color = mix(ink.rgb, vec3(0.78, 0.96, 1.0), beam*envelope*0.6);
    }
    alpha = clamp(alpha, 0.0, 0.8);
    fragColor = vec4(color*alpha, alpha)*qt_Opacity;
}
