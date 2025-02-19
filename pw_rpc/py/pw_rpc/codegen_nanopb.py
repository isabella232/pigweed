# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""This module generates the code for nanopb-based pw_rpc services."""

import os
from typing import Iterable, Iterator, NamedTuple, Optional

from pw_protobuf.output_file import OutputFile
from pw_protobuf.proto_tree import ProtoNode, ProtoService, ProtoServiceMethod
from pw_protobuf.proto_tree import build_node_tree
from pw_rpc import codegen
from pw_rpc.codegen import RPC_NAMESPACE
import pw_rpc.ids

PROTO_H_EXTENSION = '.pb.h'
PROTO_CC_EXTENSION = '.pb.cc'
NANOPB_H_EXTENSION = '.pb.h'


def _proto_filename_to_nanopb_header(proto_file: str) -> str:
    """Returns the generated nanopb header name for a .proto file."""
    return os.path.splitext(proto_file)[0] + NANOPB_H_EXTENSION


def _proto_filename_to_generated_header(proto_file: str) -> str:
    """Returns the generated C++ RPC header name for a .proto file."""
    filename = os.path.splitext(proto_file)[0]
    return f'{filename}.rpc{PROTO_H_EXTENSION}'


def _generate_method_descriptor(method: ProtoServiceMethod, method_id: int,
                                output: OutputFile) -> None:
    """Generates a nanopb method descriptor for an RPC method."""

    req_fields = f'{method.request_type().nanopb_name()}_fields'
    res_fields = f'{method.response_type().nanopb_name()}_fields'
    impl_method = f'&Implementation::{method.name()}'

    output.write_line(
        f'{RPC_NAMESPACE}::internal::GetNanopbOrRawMethodFor<{impl_method}, '
        f'{method.type().cc_enum()}, '
        f'{method.request_type().nanopb_name()}, '
        f'{method.response_type().nanopb_name()}>(')
    with output.indent(4):
        output.write_line(f'0x{method_id:08x},  // Hash of "{method.name()}"')
        output.write_line(f'{req_fields},')
        output.write_line(f'{res_fields}),')


def _generate_server_writer_alias(output: OutputFile) -> None:
    output.write_line('template <typename T>')
    output.write_line(
        f'using ServerWriter = {RPC_NAMESPACE}::ServerWriter<T>;')


def _generate_code_for_service(service: ProtoService, root: ProtoNode,
                               output: OutputFile) -> None:
    """Generates a C++ derived class for a nanopb RPC service."""
    codegen.service_class(service, root, output, _generate_server_writer_alias,
                          'NanopbMethodUnion', _generate_method_descriptor)


class _CallbackFunction(NamedTuple):
    """Represents a callback function parameter in a client RPC call."""

    function_type: str
    name: str
    default_value: Optional[str] = None

    def __str__(self):
        param = f'::pw::Function<{self.function_type}> {self.name}'
        if self.default_value:
            param += f' = {self.default_value}'
        return param


def _generate_code_for_client_method(method: ProtoServiceMethod,
                                     output: OutputFile) -> None:
    """Outputs client code for a single RPC method."""

    req = method.request_type().nanopb_name()
    res = method.response_type().nanopb_name()
    method_id = pw_rpc.ids.calculate(method.name())

    rpc_error = _CallbackFunction(
        'void(::pw::Status)',
        'on_rpc_error',
        'nullptr',
    )

    if method.type() == ProtoServiceMethod.Type.UNARY:
        callbacks = f'{RPC_NAMESPACE}::internal::UnaryCallbacks'
        functions = [
            _CallbackFunction(f'void(const {res}&, ::pw::Status)',
                              'on_response'),
            rpc_error,
        ]
    elif method.type() == ProtoServiceMethod.Type.SERVER_STREAMING:
        callbacks = f'{RPC_NAMESPACE}::internal::ServerStreamingCallbacks'
        functions = [
            _CallbackFunction(f'void(const {res}&)', 'on_response'),
            _CallbackFunction('void(::pw::Status)', 'on_stream_end'),
            rpc_error,
        ]
    else:
        raise NotImplementedError(
            'Only unary and server streaming RPCs are currently supported')

    call_alias = f'{method.name()}Call'

    output.write_line()
    output.write_line(
        f'using {call_alias} = {RPC_NAMESPACE}::NanopbClientCall<')
    output.write_line(f'    {callbacks}<{res}>>;')
    output.write_line()
    output.write_line(f'static {call_alias} {method.name()}(')
    with output.indent(4):
        output.write_line(f'{RPC_NAMESPACE}::Channel& channel,')
        output.write_line(f'const {req}& request,')

        # Write out each of the callback functions for the method type.
        for i, function in enumerate(functions):
            if i == len(functions) - 1:
                output.write_line(f'{function}) {{')
            else:
                output.write_line(f'{function},')

    with output.indent():
        output.write_line(f'{call_alias} call(&channel,')
        with output.indent(len(call_alias) + 6):
            output.write_line('kServiceId,')
            output.write_line(
                f'0x{method_id:08x},  // Hash of "{method.name()}"')

            moved_functions = (f'std::move({function.name})'
                               for function in functions)
            output.write_line(f'{callbacks}({", ".join(moved_functions)}),')

            output.write_line(f'{req}_fields,')
            output.write_line(f'{res}_fields);')
        output.write_line('call.SendRequest(&request);')
        output.write_line('return call;')

    output.write_line('}')


def _generate_code_for_client(service: ProtoService, root: ProtoNode,
                              output: OutputFile) -> None:
    """Outputs client code for an RPC service."""

    output.write_line('namespace nanopb {')

    class_name = f'{service.cpp_namespace(root)}Client'
    output.write_line(f'\nclass {class_name} {{')
    output.write_line(' public:')

    with output.indent():
        output.write_line(f'{class_name}() = delete;')

        for method in service.methods():
            _generate_code_for_client_method(method, output)

    service_name_hash = pw_rpc.ids.calculate(service.proto_path())
    output.write_line('\n private:')

    with output.indent():
        output.write_line(f'// Hash of "{service.proto_path()}".')
        output.write_line(
            f'static constexpr uint32_t kServiceId = 0x{service_name_hash:08x};'
        )

    output.write_line('};')

    output.write_line('\n}  // namespace nanopb\n')


def includes(proto_file, unused_package: ProtoNode) -> Iterator[str]:
    yield '#include "pw_rpc/internal/nanopb_method_union.h"'
    yield '#include "pw_rpc/nanopb_client_call.h"'

    # Include the corresponding nanopb header file for this proto file, in which
    # the file's messages and enums are generated. All other files imported from
    # the .proto file are #included in there.
    nanopb_header = _proto_filename_to_nanopb_header(proto_file.name)
    yield f'#include "{nanopb_header}"'


def _generate_code_for_package(proto_file, package: ProtoNode,
                               output: OutputFile) -> None:
    """Generates code for a header file corresponding to a .proto file."""

    codegen.package(proto_file, package, output, includes,
                    _generate_code_for_service, _generate_code_for_client)


class StubGenerator(codegen.StubGenerator):
    def unary_signature(self, method: ProtoServiceMethod, prefix: str) -> str:
        return (f'::pw::Status {prefix}{method.name()}(ServerContext&, '
                f'const {method.request_type().nanopb_name()}& request, '
                f'{method.response_type().nanopb_name()}& response)')

    def unary_stub(self, method: ProtoServiceMethod,
                   output: OutputFile) -> None:
        output.write_line(codegen.STUB_REQUEST_TODO)
        output.write_line('static_cast<void>(request);')
        output.write_line(codegen.STUB_RESPONSE_TODO)
        output.write_line('static_cast<void>(response);')
        output.write_line('return ::pw::Status::Unimplemented();')

    def server_streaming_signature(self, method: ProtoServiceMethod,
                                   prefix: str) -> str:
        return (
            f'void {prefix}{method.name()}(ServerContext&, '
            f'const {method.request_type().nanopb_name()}& request, '
            f'ServerWriter<{method.response_type().nanopb_name()}>& writer)')


def process_proto_file(proto_file) -> Iterable[OutputFile]:
    """Generates code for a single .proto file."""

    _, package_root = build_node_tree(proto_file)
    output_filename = _proto_filename_to_generated_header(proto_file.name)
    output_file = OutputFile(output_filename)
    _generate_code_for_package(proto_file, package_root, output_file)

    output_file.write_line()
    codegen.package_stubs(package_root, output_file, StubGenerator())

    return [output_file]
