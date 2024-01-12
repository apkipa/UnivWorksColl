package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.annotation.SaCheckLogin;
import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.UserRelationshipDraft;
import com.apkipa.tweetsystem.repository.UserRelationshipRepository;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;

@RestController
@RequestMapping("/api/v1/user-relationship")
public class UserRelationshipController {
    @Autowired
    UserRepository userRepository;

    @Autowired
    UserRelationshipRepository userRelationshipRepository;

    @SaCheckLogin
    @GetMapping("/list")
    public JResp<?> list(
            @RequestParam(required = false) Long id
    ) {
        if (id == null) {
            id = StpUtil.getLoginIdAsLong();
        }
        var user = userRelationshipRepository.findAllByUserId(id);
        return JResp.ok(user);
    }

    @SaCheckLogin
    @GetMapping("/list-inverse")
    public JResp<?> listInverse(
            @RequestParam(required = false) Long id
    ) {
        if (id == null) {
            id = StpUtil.getLoginIdAsLong();
        }
        var user = userRelationshipRepository.findAllByTargetUserId(id);
        return JResp.ok(user);
    }

    @SaCheckLogin
    @GetMapping("/get-two-relation")
    public JResp<?> getTwoRelation(
            @RequestParam(required = false) Long from,
            @RequestParam(required = false) Long to
    ) {
        if (from == null) {
            from = StpUtil.getLoginIdAsLong();
        }
        if (to == null) {
            to = StpUtil.getLoginIdAsLong();
        }
        var relInfoOpt = userRelationshipRepository.findByUserIdAndTargetUserId(from, to);
        var data = new HashMap<String, Object>();
        if (relInfoOpt.isEmpty()) {
            data.put("has_relationship", false);
            data.put("is_following", false);
            data.put("is_blocking", false);
        }
        else {
            var relInfo = relInfoOpt.get();
            data.put("has_relationship", false);
            data.put("is_following", !relInfo.isBlock());
            data.put("is_blocking", relInfo.isBlock());
        }
        return JResp.ok(data);
    }

    @SaCheckLogin
    @PostMapping("/follow")
    public JResp<?> follow(
            @RequestParam Long target
    ) {
        var id = StpUtil.getLoginIdAsLong();
        if (target.equals(id)) {
            return JResp.err(-1, "不能关注自己");
        }
        var thisUser = userRepository.findNullable(id);
        var targetUser = userRepository.findNullable(target);
        var oldUserRel = userRelationshipRepository.findNullableByUserIdAndTargetUserId(id, target);
        var newUserRel = UserRelationshipDraft.$.produce(oldUserRel, draft -> {
            draft.setUser(thisUser);
            draft.setTargetUser(targetUser);
            draft.setBlock(false);
        });
        userRelationshipRepository.save(newUserRel);
        return JResp.ok(null);
    }

    @SaCheckLogin
    @PostMapping("/block")
    public JResp<?> block(
            @RequestParam Long target
    ) {
        var id = StpUtil.getLoginIdAsLong();
        if (target.equals(id)) {
            return JResp.err(-1, "不能拉黑自己");
        }
        var thisUser = userRepository.findNullable(id);
        var targetUser = userRepository.findNullable(target);
        var oldUserRel = userRelationshipRepository.findNullableByUserIdAndTargetUserId(id, target);
        var newUserRel = UserRelationshipDraft.$.produce(oldUserRel, draft -> {
            draft.setUser(thisUser);
            draft.setTargetUser(targetUser);
            draft.setBlock(true);
        });
        userRelationshipRepository.save(newUserRel);
        return JResp.ok(null);
    }

    @SaCheckLogin
    @PostMapping("/unfollow")
    public JResp<?> unfollow(
            @RequestParam Long target
    ) {
        var id = StpUtil.getLoginIdAsLong();
        var oldUserRel = userRelationshipRepository.findByUserIdAndTargetUserId(id, target);
        if (oldUserRel.isEmpty()) {
            return JResp.err(-1, "尚未关注此用户");
        }
        userRelationshipRepository.delete(oldUserRel.get());
        return JResp.ok(null);
    }
}
