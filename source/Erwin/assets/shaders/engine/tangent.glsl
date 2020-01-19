// Compute a TBN matrix for tangent space to view space transformation
mat3 TBN(mat3 normal_matrix, vec3 normal, vec3 tangent)
{
    vec3 N = normalize(normal_matrix * normal);
    vec3 T = normalize(normal_matrix * tangent);
    // Re-orthogonalize T with respect to N by the Gram-Schmidt process
    T = normalize(T - dot(T, N)*N);
    vec3 B = -cross(N, T); // TMP: Minus sign needed atm, for normal/parallax mapping to work correctly
    return mat3(T,B,N);
}