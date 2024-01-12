package com.apkipa.tweetsystem.result;

import lombok.Getter;

public class JResp<T> {
    private T data;
    private int code;
    private String msg;

    public JResp() {
        this(0, "");
    }
    public JResp(int code, String msg) {
        this.code = code;
        this.msg = msg;
    }
    public JResp(T data) {
        this();
        this.data = data;
    }

    static public<T> JResp<T> ok(T data) {
        return new JResp<T>(data);
    }
    static public JResp<?> err(int code, String msg) {
        return new JResp<>(code, msg);
    }

    public T getData() {
        return data;
    }

    public int getCode() {
        return code;
    }

    public String getMsg() {
        return msg;
    }
}
