"use strict";

var chatHub = new signalR.HubConnectionBuilder().withUrl("/api/hubs/chatHub").build();

window.chatHub = chatHub;

$((function () {
    chatHub.start().then(function () {
        console.log("Connection started");
    }).catch(function (err) {
        return console.error(err.toString());
    });
}));
