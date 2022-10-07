
template <typename T>
struct TableRow
{
    TableRow(char* ptr, size_t numOfBytes) : 
        begin(reinterpret_cast<T*>(ptr)),
        end(reinterpret_cast<T*>(ptr + numOfBytes * sizeof(T)))
        {   }
        
    T *begin{nullptr};
    T *end{nullptr};
};