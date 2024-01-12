package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.annotation.SaCheckLogin;
import cn.dev33.satoken.secure.SaSecureUtil;
import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.ERole;
import com.apkipa.tweetsystem.model.UserDraft;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import com.ctc.wstx.sw.EncodingXmlWriter;
import org.jetbrains.annotations.Nullable;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.sql.Timestamp;
import java.time.Instant;

@RestController
@RequestMapping("/api/v1/auth")
public class AuthController {
    @Autowired
    UserRepository userRepository;

    static String EncryptPassword(String password) {
        return SaSecureUtil.sha256(password);
    }

    @PostMapping("/login")
    public JResp<?> login(
            @RequestParam String username,
            @RequestParam String password
    ) {
        var userOpt = userRepository.findByName(username);
        if (userOpt.isEmpty()) {
            return JResp.err(-1, "用户名或密码错误");
        }
        var user = userOpt.get();
        if (EncryptPassword(password).equals(user.password())) {
            StpUtil.login(user.id());
            return JResp.ok(StpUtil.getTokenInfo());
        }
        return JResp.err(-1, "用户名或密码错误");
    }

    @SaCheckLogin
    @PostMapping("/logout")
    public JResp<?> logout() {
        StpUtil.logout();
        return JResp.ok(null);
    }

    @PostMapping("/register")
    public JResp<?> register(
            @RequestParam String username,
            @RequestParam String password
    ) {
        if (username.isEmpty()) {
            return JResp.err(-1, "用户名不得为空");
        }
        if (password.isEmpty()) {
            return JResp.err(-1, "密码不得为空");
        }
        if (userRepository.existsByName(username)) {
            return JResp.err(-1, "用户名已被占用");
        }
        var user = UserDraft.$.produce(draft -> {
            draft.setName(username);
            draft.setPassword(EncryptPassword(password));
            draft.setNickname(username);
            draft.setSex("None");
            draft.setRegTime(Timestamp.from(Instant.now()));
            draft.setAge(null);
            draft.setEmail("");
            draft.setIntroduction("");
            draft.setRole(ERole.ROLE_USER);
        });
        userRepository.save(user);
        return JResp.ok(null);
    }

    @SaCheckLogin
    @PostMapping("/update-info")
    public JResp<?> updateInfo(
            @RequestParam(required = false) String nickname,
            @RequestParam(required = false) String introduction,
            @RequestParam(required = false) String password,
            @RequestParam(required = false) String sex,
            @RequestParam(required = false) Integer age,
            @RequestParam(required = false) String email
    ) {
        var userOpt = userRepository.findById(StpUtil.getLoginIdAsLong());
        if (userOpt.isEmpty()) {
            return JResp.err(-1, "无效用户");
        }
        var user = UserDraft.$.produce(userOpt.get(), draft -> {
            if (nickname != null) {
                draft.setNickname(nickname);
            }
            if (introduction != null) {
                draft.setIntroduction(introduction);
            }
            if (password != null) {
                draft.setPassword(EncryptPassword(password));
            }
            if (sex != null) {
                draft.setSex(sex);
            }
            if (age != null) {
                draft.setAge(age);
            }
            if (email != null) {
                draft.setEmail(email);
            }
        });
        userRepository.save(user);
        return JResp.ok(null);
    }
}
