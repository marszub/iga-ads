#ifndef ADS_OUTPUT_VTK_HPP_
#define ADS_OUTPUT_VTK_HPP_

#include <boost/format.hpp>
#include "ads/output/output_base.hpp"
#include "ads/output/grid.hpp"
#include "ads/lin/tensor/base.hpp"
#include "ads/util/io.hpp"


namespace ads {
namespace output {

namespace impl {

template <typename... Iters>
struct vtk_print_helper : output_base {

    vtk_print_helper(const output_format& format)
    : output_base { format }
    { }

    template <typename Value, typename... Values>
    void print(std::ostream& os, const grid<Iters...>&, const Value& value, const Values&... values) {
        util::stream_state_saver guard(os);
        prepare_stream(os);
        for_each_multiindex([this,&os,&value,&values...](auto... is) {
            this->print_row(os, value(is...), values(is...)...);
        }, value);
    }

};

template <
    typename T,
    std::size_t Rank,
    std::size_t I,
    typename Tensor,
    typename... Indices
>
struct tensor_helper {

    using Next = tensor_helper<T, Rank, I + 1, Tensor, std::size_t, Indices...>;

    template <typename F>
    static void for_each_multiindex(F fun, const Tensor& t, Indices... indices) {
        auto size = t.size(I);
        for (std::size_t i = 0; i < size; ++ i) {
            Next::for_each_multiindex(fun, t, i, indices...);
        }
    }
};

template <
    typename T,
    std::size_t Rank,
    typename Tensor,
    typename... Indices
>
struct tensor_helper<T, Rank, Rank, Tensor, Indices...> {

    template <typename F>
    static void for_each_multiindex(F fun, const Tensor&, Indices... indices) {
        fun(indices...);
    }
};

template <typename T, std::size_t Rank, typename Impl, typename F>
void for_each_multiindex(F fun, const lin::tensor_base<T, Rank, Impl>& t) {
    tensor_helper<T, Rank, 0, lin::tensor_base<T, Rank, Impl>>::for_each_multiindex(fun, t);
}

}

struct vtk : output_base {

    vtk(const output_format& format)
    : output_base { format }
    { }

    template <typename... Iters, typename... Values>
    void print(std::ostream& os, const grid<Iters...>& grid, const Values&... values) {
        using boost::format;
        constexpr auto dim = sizeof...(Iters);
        auto origin = repeat(0, dim);
        auto extent = make_extent(dims(grid));
        auto spacing = repeat(1, dim);
        auto value_count = sizeof...(Values);

        os << "<?xml version=\"1.0\"?>" << std::endl;
        os << "<VTKFile type=\"ImageData\" version=\"0.1\">" << std::endl;
        os << format("  <ImageData WholeExtent=\"%s\" origin=\"%s\" spacing=\"%s\">")
                % extent % origin % spacing<< std::endl;
        os << format("    <Piece Extent=\"%s\">") % extent << std::endl;
        os <<        "      <PointData Scalars=\"Result\">" << std::endl;
        os << format("        <DataArray Name=\"Result\"  type=\"Float32\" "
                "format=\"ascii\" NumberOfComponents=\"%d\">") % value_count << std::endl;
        impl::vtk_print_helper<Iters...> printer { output_base::format };
        printer.print(os, grid, values...);
        // data
        os << "        </DataArray>" << std::endl;
        os << "      </PointData>" << std::endl;
        os << "    </Piece>" << std::endl;
        os << "  </ImageData>" << std::endl;
        os << "</VTKFile>" << std::endl;
    }

private:

    template <typename T>
    std::string repeat(T value, std::size_t n) {
        return join(' ', std::vector<T>(n, value));
    }

    template <typename T, std::size_t N>
    std::string make_extent(const std::array<T, N>& extents) {
        std::vector<T> exts(2 * N);
        for (std::size_t i = 0; i < N; ++ i) {
            exts[2 * i + 1] = extents[i] - 1;
        }
        return join(' ', exts);
    }

    template <typename Sep, typename Cont>
    std::string join(Sep sep, const Cont& cont) {
        using std::begin;
        using std::endl;
        return join(sep, begin(cont), end(cont));
    }

    template <typename Sep, typename Iter>
    std::string join(Sep sep, Iter begin, Iter end) {
        std::stringstream ss;
        while (true) {
            ss << *begin++;
            if (begin != end) {
                ss << sep;
            } else {
                break;
            }
        }
        return ss.str();
    }

};


}
}



#endif /* ADS_OUTPUT_VTK_HPP_ */