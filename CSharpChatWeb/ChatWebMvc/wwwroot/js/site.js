// Please see documentation at https://learn.microsoft.com/aspnet/core/client-side/bundling-and-minification
// for details on configuring this project to bundle and minify static web assets.

// Write your JavaScript code.

function unwrapApiJson(data) {
    console.log(data);
    const code = data.code ?? data.status;
    const msg = data.msg ?? data.errors;
    if (code >= 0) {
        return data.data;
    }
    // Otherwise, throw with data.code and data.msg
    throw new Error(`${code}: ${msg}`);
}
