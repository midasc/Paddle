/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include "paddle/operators/transpose_op.h"

namespace paddle {
namespace operators {

using framework::Tensor;

class TransposeOp : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(const framework::InferShapeContext &ctx) const override {
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar("X"), "Input(X) should not be null");
    PADDLE_ENFORCE_NOT_NULL(ctx.OutputVar("Out"),
                            "Output(Out) should not be null");
    auto x_dims = ctx.Input<Tensor>("X")->dims();
    std::vector<int> axis = ctx.Attr<std::vector<int>>("axis");
    size_t x_rank = x_dims.size();
    size_t axis_size = axis.size();

    PADDLE_ENFORCE_EQ(x_rank, axis_size,
                      "the input tensor's rank(%d) "
                      "should be equal to the axis's size(%d)",
                      x_rank, axis_size);

    std::vector<int> count(axis_size, 0);
    for (size_t i = 0; i < axis_size; i++) {
      PADDLE_ENFORCE(
          axis[i] < static_cast<int>(axis_size) && ++count[axis[i]] == 1,
          "Each element of Attribute axis should be a unique value "
          "range from 0 to (dims - 1), "
          "where the dims is the axis's size");
    }

    framework::DDim out_dims(x_dims);
    for (size_t i = 0; i < axis_size; i++) {
      out_dims[i] = x_dims[axis[i]];
    }
    ctx.Output<framework::LoDTensor>("Out")->Resize(out_dims);
  }
};

class TransposeOpMaker : public framework::OpProtoAndCheckerMaker {
 public:
  TransposeOpMaker(framework::OpProto *proto,
                   framework::OpAttrChecker *op_checker)
      : OpProtoAndCheckerMaker(proto, op_checker) {
    AddInput(
        "X",
        "(Tensor)The input tensor, tensors with rank at most 6 are supported");
    AddOutput("Out", "(Tensor)The output tensor");
    AddAttr<std::vector<int>>(
        "axis",
        "(vector<int>)a list of values, and the size of the list should be "
        "the same with the input tensor rank, the tensor will "
        "permute the axes according the the values given");
    AddComment(R"DOC(
The Tensor will be permuted according to the axis values given.
The op is very much like the numpy.transpose function in python
For example:
 >> input = numpy.arange(6).reshape((2,3))
 >> input
 array([[0, 1, 2],
        [3, 4, 5]])
 >> axis = [1, 0]
 >> output = input.transpose(axis)
 >> output 
 array([[0, 3],
        [1, 4],
		[2, 5]])
So, given a input tensor of shape(N, C, H, W) and the axis is {0, 2, 3, 1},
the output tensor shape will be (N, H, W, C)
)DOC");
  }
};

class TransposeOpGrad : public framework::OperatorWithKernel {
 public:
  using framework::OperatorWithKernel::OperatorWithKernel;

 protected:
  void InferShape(const framework::InferShapeContext &ctx) const override {
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar("X"), "Input(X) should not be null");
    PADDLE_ENFORCE_NOT_NULL(ctx.InputVar(framework::GradVarName("Out")),
                            "Input(Out@GRAD) should not be null");
    auto x_dims = ctx.Input<Tensor>("X")->dims();
    auto *x_grad =
        ctx.Output<framework::LoDTensor>(framework::GradVarName("X"));

    if (x_grad) x_grad->Resize(x_dims);
  }
};

}  // namespace operators
}  // namespace paddle

namespace ops = paddle::operators;
REGISTER_OP(transpose, ops::TransposeOp, ops::TransposeOpMaker, transpose_grad,
            ops::TransposeOpGrad);
REGISTER_OP_CPU_KERNEL(transpose,
                       ops::TransposeKernel<paddle::platform::CPUPlace, float>);
REGISTER_OP_CPU_KERNEL(
    transpose_grad,
    ops::TransposeGradKernel<paddle::platform::CPUPlace, float>);
