// inside VertexInitData definition (after DISABLE_COPY(...))
VertexInitData(VertexInitData&&) noexcept = default;
VertexInitData& operator=(VertexInitData&&) noexcept = default;