#version 330
precision mediump float;

varying vec2 fragTexCoord;
uniform float time;

void main()
{
    // normalize coordinates to [-1,1]
    vec2 uv = fragTexCoord * 2.0 - 1.0;

    // subtle moving gradient: slowly oscillate colors over time
    float dist = length(uv);
    vec3 color1 = vec3(0.02, 0.0, 0.05);
    vec3 color2 = vec3(0.0, 0.0, 0.15);
    float shift = 0.01 * sin(time * 0.05); // very slow shift
    vec3 baseColor = mix(color1, color2 + shift, dist);

    // very subtle stars
    float starPattern = fract(sin(dot(uv * 100.0, vec2(12.9898,78.233))) * 43758.5453);
    float stars = step(0.9985, starPattern);

    // make stars gently fade in/out with slow sine
    float starBrightness = stars * (0.3 + 0.2 * sin(time + uv.x * 5.0));

    baseColor += vec3(starBrightness);

    gl_FragColor = vec4(baseColor, 1.0);
}
