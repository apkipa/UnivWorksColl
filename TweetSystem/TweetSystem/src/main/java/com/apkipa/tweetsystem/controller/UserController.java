package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.annotation.SaCheckLogin;
import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.UserDraft;
import com.apkipa.tweetsystem.model.UserFetcher;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/v1/user")
public class UserController {
    @Autowired
    UserRepository userRepository;

    @SaCheckLogin
    @GetMapping("/get-info")
    public JResp<?> getUserInfo(
            @RequestParam(required = false) Long id
    ) {
//        if (id != null && !id.equals(StpUtil.getLoginIdAsLong())) {
//            return JResp.err(-1, "平行越权访问");
//        }
        if (id == null) {
            id = StpUtil.getLoginIdAsLong();
        }
        var user = userRepository.findById(id, UserFetcher.$
                .allScalarFields()
                .password(false)
                .postRecommendation(false)
        );
        if (user.isEmpty()) {
            return JResp.err(-1, "用户不存在");
        }
        return JResp.ok(user.get());
    }

    @SaCheckLogin
    @GetMapping("/get-info-by-name")
    public JResp<?> getUserInfoByName(
            @RequestParam() String name
    ) {
        var user = userRepository.findByName(name, UserFetcher.$
                .allScalarFields()
                .password(false)
                .postRecommendation(false)
        );
        if (user.isEmpty()) {
            return JResp.err(-1, "用户不存在");
        }
        return JResp.ok(user.get());
    }

    @SaCheckLogin
    @GetMapping("/search")
    public JResp<?> search(@RequestParam String name) {
        var users = userRepository.searchByNames(name, UserFetcher.$
                .name()
                .nickname()
        );
        return JResp.ok(users);
    }
}
