package com.apkipa.tweetsystem.controller;

import cn.dev33.satoken.stp.StpUtil;
import com.apkipa.tweetsystem.model.CollectionFetcher;
import com.apkipa.tweetsystem.model.LikeFetcher;
import com.apkipa.tweetsystem.model.PostFetcher;
import com.apkipa.tweetsystem.model.UserFetcher;
import com.apkipa.tweetsystem.repository.PostRepository;
import com.apkipa.tweetsystem.repository.UserRelationshipRepository;
import com.apkipa.tweetsystem.repository.UserRepository;
import com.apkipa.tweetsystem.result.JResp;
import org.babyfish.jimmer.spring.model.SortUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.PageRequest;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/v1/post-recommend")
public class PostRecommendationController {
    @Autowired
    UserRepository userRepository;
    @Autowired
    UserRelationshipRepository userRelationshipRepository;
    @Autowired
    PostRepository postRepository;

    @GetMapping("/generate")
    public JResp<?> generate(
            @RequestParam Integer pn,
            @RequestParam Integer ps
    ) {
        // Generate posts from all following users
        var userId = StpUtil.getLoginIdAsLong();
//        var user = userRepository.findNullable(userId);
        var followingIds = userRelationshipRepository.listAllFollowingIdByUserId(userId);
        var posts = postRepository.listPassedByUserIds(
                PageRequest.of(pn, ps, SortUtils.toSort("publishTime desc")),
                followingIds,
                PostFetcher.$
                        .allScalarFields()
                        .user(UserFetcher.$.name().nickname())
                        .likes(LikeFetcher.$.user(UserFetcher.$))
                        .collections(CollectionFetcher.$.user())
                        .replyPosts()
        );
        return JResp.ok(posts);
    }
}
